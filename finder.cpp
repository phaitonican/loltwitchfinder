#include <iostream>
#include <string>
#include <sstream>

#include <curl/curl.h>
#include <json/json.h>
#include <vector>

namespace
{
    std::size_t callback(
            const char* in,
            std::size_t size,
            std::size_t num,
            char* out)
    {
        std::string data(in, (std::size_t) size * num);
        *((std::stringstream*) out) << data;
        return size * num;        
    }

    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty())
            return;
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }
}

int main(int argc, char *argv[])
{
    const std::string riot_api_key = "RGAPI-f3c70eee-ca25-49ed-a3db-dae8e49ca445"; // Riot API key (from riot site)
    const char *twitch_client_id = "Client-ID: 1ivst1y1o0idnxke9w1ua4to59tx7g"; // Client ID of twitch API (from twitch site)
    std::string summoner_name = "phaitonican"; // Default summoner name

    if(argv[1] != NULL)
    {
        summoner_name = argv[1];
    } else
    {
        std::cout << "No name defined. Searching for default name." << std::endl;
    }

    std::string summonerID;
    std::vector<std::string> participants(10);
    std::string errs;

    CURL* curl = curl_easy_init();

    //first request (riot games)
    char *s_sum = curl_easy_escape(curl, summoner_name.c_str(), summoner_name.length());
    std::string str_sum(s_sum);
    curl_free(s_sum);
    const std::string url_summoner("https://euw1.api.riotgames.com/lol/summoner/v4/summoners/by-name/" + str_sum + "?api_key=" + riot_api_key);
    
    curl_easy_setopt(curl, CURLOPT_URL, url_summoner.c_str());

    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Response information.
    long httpCode(0);
    std::stringstream httpData;

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpData);

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    const Json::CharReaderBuilder jsonReader;
    Json::Value jsonData;

    if (httpCode == 200)
    {
        std::cout << "Player " + summoner_name + " found." << std::endl;

        if (Json::parseFromStream(jsonReader, httpData, &jsonData, &errs))
        {
            summonerID = jsonData["id"].asString();
        }
        else
        {
            std::cout << "Could not parse HTTP data as JSON" << std::endl;
            std::cout << "HTTP data was:\n" << httpData.str() << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "No Player found. Exiting." << std::endl;
        return 1;
    }

    // Setup second request (riot games)
    httpData.str(std::string());

    const std::string url_spectator("https://euw1.api.riotgames.com/lol/spectator/v4/active-games/by-summoner/" + summonerID + "?api_key=" + riot_api_key);
    
    curl_easy_setopt(curl, CURLOPT_URL, url_spectator.c_str());

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpData);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    if (httpCode == 200)
    {
        std::cout << "Active game found.\n" << std::endl;
        jsonData.clear();

        if (Json::parseFromStream(jsonReader, httpData, &jsonData, &errs))
        {
            for(int i = 0; i < participants.size(); i++)
            {
                Json::Value json = jsonData["participants"][i]["summonerName"];
                
                participants[i] = json.asString();
            }
        }
        else
        {
            std::cout << "Could not parse HTTP data as JSON" << std::endl;
            std::cout << "HTTP data was:\n" << httpData.str() << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "No Active Game found. Exiting." << std::endl;
        return 1;
    }

    // Setup third request (twitch)

    /* Add a custom header */ 
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, twitch_client_id);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  
    curl_easy_setopt(curl, CURLOPT_URL, "localhost");
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    //For Loop for all Players

    for(int i = 0; i < participants.size(); i++)
    {
        httpData.str(std::string());

        std::string part_char = participants[i];
        replaceAll(part_char, " ", "_");

        char *s= curl_easy_escape(curl, part_char.c_str(), part_char.length());
        std::string str_twitch(s);
        curl_free(s);

        // std::cout << str_twitch << std::endl;

        const std::string url_twitch("https://api.twitch.tv/helix/streams?user_login=" + str_twitch);

        curl_easy_setopt(curl, CURLOPT_URL, url_twitch.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpData);

        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        if (httpCode == 200)
        {
            jsonData.clear();

            if (Json::parseFromStream(jsonReader, httpData, &jsonData, &errs))
            {
                // std::cout << jsonData.toStyledString() << std::endl;
                if(!jsonData["data"].empty() and jsonData["data"][0]["type"] == "live")
                {
                    std::cout << participants[i] + " found: https://twitch.tv/" + str_twitch + "\033[1;31m IS LIVE!\033[0m" << std::endl;
                } else
                {
                    std::cout << participants[i] + " found: https://twitch.tv/" + str_twitch << std::endl;
                }
            }
            else
            {
                std::cout << "Could not parse HTTP data as JSON" << std::endl;
                std::cout << "HTTP data was:\n" << httpData.str() << std::endl;
            }
        }
        else
        {
            std::cout << participants[i] + " not found." << std::endl;
        }
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(chunk);
}