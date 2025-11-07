#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

struct httpRequest {
    /** Service name for request redirection (/service-name/endpoint ,/order-service/orders/123). */
    std::string service_name;

    /** HTTP method (GET, POST, PUT, DELETE, etc.) */
    std::string method;

    /** Request path (e.g., /api/orders/123) */
    std::string path;

    /** HTTP headers (key-value pairs) */
    std::map<std::string, std::string> headers;

    /** Request payload (JSON, text, or form data) */
    std::string body;

    std::string target_url;
    std::string exposure; 
    bool requires_auth;
};

struct customRoute {
    std::string r_path;   // Route path (e.g., "/api/users")
    std::string r_method; // HTTP method (e.g., "GET", "POST")
    std::string r_exp;    // Exposure control (PUBLIC/PRIVATE)
    bool r_auth;          // Whether auth verification is required
};

struct serviceConfig {
    std::string s_name;  // Service name
    std::string s_host;  // Hostname or IP
    int s_port;          // TCP port
    std::string d_exp;   //Default exposure
    bool d_auth;         // Default auth requirement
    std::map<std::string, customRoute> custom_routes;
};

//----------------------------------------LOAD_CONFIG------------------------------------------------//

/** Most important map, stores all gateway configuration. */
std::map<std::string, serviceConfig> _service;

std::string extract_section(const std::string &line) {
    if (line.size() < 3) return "";
    if (line.front() == '['  && line.back() == ']') {
        return line.substr(1, line.size() - 2);
    }
    return "";
}

/**
 * Reads configuration from 'config.txt' file, saves configuration related
 * to multiple services and routes related to service
 */
void load_config(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "FAILED TO OPEN FILE: " << filename << "\n";
    }

    std::string line;
    serviceConfig curr_service;
    customRoute curr_route;
    bool in_route_section = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip comments or blank lines

        /* Gets us the content inside the [] brackets */
        std::string section = extract_section(line);

        if (!section.empty()) {
            /** Checks if the section is route or not, if route then true or false. */
            if (section.find(".routes") == std::string::npos) {
                /**
                 * If current service config is not empty it means
                 * this is not the first service saved.
                 */
                if (!curr_service.s_name.empty()) {
                    /**
                     * Save the current service config and create new service config struct
                     */
                    _service[curr_service.s_name] = curr_service;
                    curr_service = serviceConfig();
                }
                /**
                 * Not in route section, save current service name.
                 */
                in_route_section = false;
                curr_service.s_name = section;
            } else {
                in_route_section = true;
            }
            continue;
        }

        std::size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        if (!in_route_section) {
            if (key == "HOST") curr_service.s_host = value;
            else if (key == "PORT") curr_service.s_port = std::stoi(value);
            else if (key == "DEFAULT-EXPOSURE") curr_service.d_exp = value;
            else if (key == "DEFAULT-AUTH") curr_service.d_auth = (value == "TRUE" || value == "true");
        } else {
            if (key == "PATH") curr_route.r_path = value;
            else if (key == "METHOD") curr_route.r_method = value;
            else if (key == "EXPOSURE") curr_route.r_exp = value;
            else if (key == "AUTH") curr_route.r_auth = (value == "TRUE" || value == "true");

            /**
             * "/order-service/orders/id:GET"
             */
            if (!curr_route.r_path.empty() && !curr_route.r_method.empty()) {
                std::string routeKey = curr_route.r_path + ":" + curr_route.r_method;
                curr_service.custom_routes[routeKey] = curr_route;
                curr_route = customRoute();
            }
        }
    }

    if (!curr_service.s_name.empty()) {
        _service[curr_service.s_name] = curr_service;
    }

    file.close();

    std::cout << "[LOAD ENV] TRUE" << std::endl;
}

//------------------------------------------PATTERN_MATCHER------------------------------------------------//

/** Utility for pattern_matcher */
std::vector<std::string> split_path(const std::string &path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string segment;

    while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) parts.push_back(segment);
    }
    return parts;
}

/** Matches the provided path from request and saved path inside config 
 * If no route is matched then default attributes are set to http object.
*/
httpRequest pattern_matcher(httpRequest& inc_req){

    /** Extract attributes from HTTP Request Object. */
    std::string s_name = inc_req.service_name;
    std::string req_method = inc_req.method;
    std::string req_path = inc_req.path;

    /** Find the corresponding service. */
    auto svc_it = _service.find(s_name);
    if (svc_it == _service.end()) {
        std::cerr << "SERVICE NOT FOUND: " << s_name << "\n";
        return inc_req;
    }

    const serviceConfig &svc = svc_it->second;

    /** Split the incoming path for matching. */
    std::vector<std::string> req_split = split_path(req_path);

    /** Iterating over custom routes saved in corresponding service. */
    for(const auto& [route_key, route_value] : svc.custom_routes){

        /** Find the semicolon ':' and provide index of it , search only last occurence. */
        size_t pos = route_key.find_last_of(':');

        /** If there is no semicolon skip this route. */
        if (pos == std::string::npos) continue;

        /** Separate path and method from the saved route inside config
         * /orders/:id:GET
         */
        std::string pattern_path = route_key.substr(0, pos);
        std::string pattern_method = route_key.substr(pos + 1);

        /** Search only the method inside provided request object. */
        if (pattern_method != req_method) continue;

        /** Split the saved path inside config
         * ["orders",":id"];
        */
        std::vector<std::string> pattern_parts = split_path(pattern_path);

        /* If required path and saved path not same length skip this route. */
        if (pattern_parts.size() != req_split.size()) continue;

        bool matched = true;

        /** Check for index which contains the semicolor ':'
         * Indicating dynamic variable
         */
        for(int i = 0; i < pattern_parts.size(); i++){
            const std::string &p_seg = pattern_parts[i];
            const std::string &r_seg = req_split[i];

            if (!p_seg.empty() && p_seg[0] == ':'){
                continue; 
            } else if (p_seg != r_seg) {
                matched = false;
                break;
            }   
        }
        if (matched) {
            std::ostringstream final_url;
            final_url << "http://" << svc.s_host << ":" << svc.s_port << req_path;
            inc_req.target_url = final_url.str();

            return inc_req;
        }
    }

    std::ostringstream default_url;
    default_url << "http://" << svc.s_host << ":" << svc.s_port << req_path;
    inc_req.exposure = svc.d_exp;
    inc_req.requires_auth = svc.d_auth;
    inc_req.target_url = default_url.str();
    return inc_req;
}
//------------------------------------------ROUTER------------------------------------------------//

/**
 * Send request to the required service, all attributes are provided in request object.
 * 1. Authentication (if required)
 * 2. Logging
 */
httpRequest router(httpRequest& req){
    /** Logger */
    std::cout << "[ROUTER] " << req.method << " " << req.path 
        << " -> " << req.target_url
        << " (Exposure=" << req.exposure 
        << ", Auth=" << std::boolalpha << req.requires_auth << ")\n";
    
    /** If authentication is required then send this request to auth first
     * if confirmation is recevied only then send request to the required service
     * else deny access.
     */
    if(req.requires_auth == true){
        /** HTTP Client for sending request to auth service */
    }else{
        /** Directly send to the service */
    }
}

httpRequest build_http_request(const std::string& s_name, const std::string path, const std::string method, const std::map<std::string,std::string>& headers, const std::string& body = ""){
    httpRequest req;
    req.service_name = s_name;
    req.method = method;
    req.path = path;
    req.headers = headers;
    req.body = body;
    req.exposure = "PUBLIC";
    req.requires_auth = false;
    req.target_url = ""; 
    return req;
}
//------------------------------------------MAIN------------------------------------------------//

int main() {
    load_config("./config.txt");
    return 0;
}
