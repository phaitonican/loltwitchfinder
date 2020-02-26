#  League of Legends Twitch Finder
This little C++ program tries to find the Twitch accounts of a live game of the desired summoner. Use like `./finder <summoner_name>` (in quotes if with spaces).

# Build
Clone the repository. Change the **riot_api_key** and the **twitch_client_id** in *finder.cpp* (my riot key regenerates every 24 hours, so better you have your own Riot and Twitch API keys).

[https://developer.riotgames.com/](https://developer.riotgames.com/), [https://dev.twitch.tv/](https://dev.twitch.tv/)

Then `g++ finder.cpp -o finder -lcurl -ljsoncpp`

## Dependencies
libcurl, jsoncpp
