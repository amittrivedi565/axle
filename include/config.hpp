#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <unordered_map>

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
        std::unordered_map<std::string, std::string> _data_;
};
#endif