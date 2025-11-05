//copied from my first assignment. finally figured out how to make it work as a seperate file
#pragma once
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;
//logging assignment stuff
struct logMessage {
    string m;
    int level;
    //level = 0 -> Event
    //level = 1 -> Info
    //level = 2 -> Error

};

class Logger {
    private: //singleton stuff
        //using geeks4geeks guide on singletons
         static Logger* instance;
        // Private constructor
        Logger();
        // Delete copy constructor and assignment
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        bool visible; //initalizes as false. Private variable so that it can't be changed without the visibility actually changing
        
        
    public:
        ofstream output;
        //for log messages
        vector<logMessage> LogMessages;
        string prefix;
       //singleton "construct"
        static Logger* getInstance();
        
        void ShowLogWindow(); //create the window
        //log visibility, feautre disabled
        //void ToggleLogWindow();
        //bool IsLogWindowVisible();
        
        //types of messages
        void LogInfo(string message); //standard info message
        void LogGameEvent(string message); //flavor message
        void LogError(string message); //non-fatal error
};
