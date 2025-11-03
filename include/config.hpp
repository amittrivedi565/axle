#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

/**
 * EXPOSURE CONTROL FOR CUSTOM ROUTE CONFIGS
 * FOR E.G., PUBLIC CAN BE ACCESSED BY ANYONE,
 * PRIVATE WILL REQUIRE AUTHENTICATION BUT CAN BE DISABLED,
 * PROTECTED ONLY LOCALNETWORK CAN ACCESS THAT.
 */
enum class Exposure{
    PUBLIC,
    PRIVATE,
    PROTECTED
};

/**
 * SERVICE CONFIGURATION WITH FIELDS SUCH AS NAME, HOST, PORT
 * DEFAULT-EXPOSURE, DEFAULT-AUTH.
 * 
 * THESE DEFAULT FIELDS WILL BE UTILIZED BY ALL THE ROUTES
 * CAN OVERRIDEN USING ROUTE-SPECIFIC CONFIGS.
 * 
 * FOR E.G, DEFAULT-EXPOSURE=PRIVATE, DEFAULT-AUTH=true
 * IF AUTH IS ENALBED AUTH SERVICE WILL BE CALLED AT EACH REQUEST
 * ONLY UPON THE CONFIRMATION BY THE AUTH-SERVICE REQUEST WILL BE FULFILLED.
 */
struct ServiceConfig {
    std::string name;
    std::string host;
    int port;
    Exposure defaultExposure;
    bool defaultAuth;
    std::map<std::string, RouteConfig> routes;
};

/**
 * ROUTE-SPECIFIC CONFIGS TO HAVE FINE GRANULAR CONTROL OVER CUSTOM ROUTES
 * PATH=/users/:id
 * METHODS=POST
 * EXPOSURE=PRIVATE
 * AUTH-REQUIRED=TRUE
 */
struct RouteConfig {
    std::string path;
    std::string method;
    Exposure exposure;
    bool authRequired;
};

class Config{
    public:
        static Config& instance(){
            static Config instance;
            return instance;
        }
        void load(const std::string filename);
        std::string get(const std::string& key) const;
        static int stringToInt(const std::string& str);
    private:
        Config(){};
            std::unordered_map<std::string, ServiceConfig> _services_;
};
#endif