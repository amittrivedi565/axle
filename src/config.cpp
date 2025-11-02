#include "config.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

/**
 * @brief LOADS CONFIGURATION FROM THE .TXT FILE
 * 
 * `ifstream` CLASS FROM `fstream` HEADER
 * USED TO READ DATA FROM THE FILES
 * 
 * `file` IS NAME OF THE OBJECT BEING CREATED, IT REPRESENTS FILE STREAM
 * `filename` IS A VARIABLE THAT CONTAINS THE FILENAME OF `config.txt` IT CONTAINS
 * PATH OR FILENAME.
 * 
 * OBJECT `file` AUTOMATICALLY TRIES TO OPEN FILE WITH THE PROVIVDED INFO
 */
void Config::load(const std::string filename){
    /**
     * FILE OBJECT CONTAINS METHOD `is_open()` BOOLEAN
     * CHECKS IF THE FILE IS OPENED SUCCESSFULLY OR NOT.
     * IF SUCCESS RETURN TRUE OR FALSE.
     * STOP THE FURTHER EXCECUTION OF PROGRAM
     */
    std::ifstream file(filename);
    if(!file.is_open()){
        std::cerr << "FAILED TO OPEN FILE" << " " << filename << "\n";
        return;
    }

    /**
     * STRING `line` CONTAINS INDIVISUAL LINE CONTENT, FOR E.G., NO.1 CONTAINS HELLOWORLD
     */
    std::string line, section;

    /**
     * `getline` READS LINE OF TEXT FROM THE FILE STREAM, STORES IT IN `line`
     * IT AUTOMATICALLY STOPES WHEN ENCOUNTERS WHEN NEW LINE CHARACTER `\n`
     * RETURNS TRUE IF SUCCESS, OR ELSE FALSE IF REACHED END OF THE FILE (EOF).
     */ 
    while(std::getline(file,line)){

        /**
         * IF THE LINE CONTAINS ANY COMMENTS STARTING WITH `#` THEN CONTINUE
         * FOR E.G., #USER SERVICE FOR ELECTRONICS
         */
        if(line.empty() || line[0] == '#') continue;

        /**
         * SECTION WILL STORE THE MICROSERVICE NAME
         * FOR E.G., 
         * [USER-SERVICE]
         * HOST=127.0.0.1
         * PORT=5001
         */
        if(line.front() == '[' && line.back() == ']'){
            section = line.substr(1, line.size() - 2);
            continue;;
        }  
        
        /**
         * EXTRACTS INFO FROM THE CURRENT LINE, ACTS MORE LIKE SMALLER INPUTSTREAM
         * `istringstream` CREATES A INPUT STREAM, WE EXTRACT WORDS AND TOKENS 
         */
        std::istringstream iss(line);
        std::string key, value;

        /**
         * `getline` READS FROM THE LEFT TILL ENCOUNTERS `=`, AND AFTER `=` READS TILL THE EOF.
         * STORES THE VALUE IN STRINGS KEY, VALUE.
         * 
         * FOR E.G., SERVICE_NAME="MEDIA_SERVICE"
         */
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
        _data_[key] = value;
    }
    }
}

/**
 * Get Key,Pair Value STORED IN MAP
 * IF ITERATOR IS NOT POINTING TO THE END RETURN VALUE OF KEY
 * ELSE RETURN EMTPY STRING.
 */
std::string Config::get(const std::string& key) const{
    auto it = _data_.find(key);
    if(it != _data_.end()) return it->second;
    return "";
}

int Config::stringToInt(const std::string& str){
    try{
        int value = std::stoi(str);
        if(value <= 0){
            throw std::invalid_argument("VALUE MUST BE POSITIVE");
        }
        return value;
    }catch(const std::exception& ex){
        std::cerr << "INVALID INTEGER" << std::endl;
        return -1;
    }
}