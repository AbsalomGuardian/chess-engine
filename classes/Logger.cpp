#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <vector>
#include "Logger.h"
#include "../imgui/imgui.h"
using namespace std;

//logging classes
//logger functions
Logger::Logger() { //private constructor
    //cout << "DEBUG: Logger created." << endl;
    //initalize member variables
    visible = false;
    prefix = "*"; //what goes before messages. has to be set here
    output;

}

//initalize static member variables
Logger* Logger::instance = nullptr;


//create using singleton
Logger* Logger::getInstance(){
    if (instance == nullptr)
            instance = new Logger();
    return instance;
}


//looking at specs don't need depth yet, so just set up the log how I want and write to it


void Logger::ShowLogWindow() {
        visible = true; //for future functionality
        //set the starting size and position, but it can still be adjusted by the player
        ImGui::SetNextWindowPos(ImVec2(114, 148), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(477, 806), ImGuiCond_Once);
        //ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);
        ImGui::Begin("Log", nullptr, ImGuiWindowFlags_MenuBar);
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.89f, 0.52f, 0.12f, 1.00f); //orange, in practice just for title
        colors[ImGuiCol_Header] = ImVec4(0.75f, 0.58f, 0.15f, 1.00f); //faded orange for when log to file is selected
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.58f, 0.15f, 1.00f); //same faded orange
        colors[ImGuiCol_Border] = ImVec4(0.29f, 0.86f, 0.12f, 0.5f); //green, half opacity
        colors[ImGuiCol_TitleBg] = ImVec4(0.29f, 0.86f, 0.12f, 1.00f); //green
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.86f, 0.12f, 1.00f); //green
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f); //pure black
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 5.0f;     // Border size for windows
        style.FrameBorderSize = 1.0f; //put borders around the different elements of the window

        //menu bar for write to file option
        ImGui::BeginMenuBar();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(74, 219, 31, 255)); //set text in menu to green

        //completely rework this button to be retroactive
        if (ImGui::MenuItem("Write to file")) { //now no longer modal, writes the entire contents of the log
            //Open the file
            output.open("GameLog.txt");
                if( !output.is_open() ){
                    cout << "Unable to open file to write log" << endl;
                    this->LogError("Unable to open file to write log.");
                } else {
                    cout << "GameLog.txt succesfully created" << endl; //don't need to tell the player this, just tell
                    //them when the process is completed
                }

                //heads each file with date and time
                time_t now = time(0);
                char* readableTime = ctime(&now);
                string readableTimeS = readableTime; //convert result to string
                output << "Game log created at " << readableTimeS << endl;
                cout << "[To File] Game log created at " << readableTimeS << endl;

                //transcribes everything in the log vector
                //doesn't use prefix, replaces color with its own prefix
                for(int i = 0; i < this->LogMessages.size(); i++) {
                    string p; //so that it isn't confused with prefix
                    if(this->LogMessages[i].level == 0) {
                        p = "[EVENT] ";
                    } else if(this->LogMessages[i].level == 1) { //set white for info
                        p = "[INFO] ";
                    } else if(this->LogMessages[i].level == 2) { //set red for error message
                        p = "[ERROR] ";
                    }
                    string message = p + this->LogMessages[i].m;
                    output << message << endl;
                }


                this->LogGameEvent("Log successfully written to build/Debug/GameLog.txt");
        }

        if(ImGui::MenuItem("Print test messages")) {
            for(int i = 1; i < 6; i++) {
                this->LogInfo("Test message " + to_string(i));
            }
        } 
        ImGui::PopStyleColor();
        ImGui::EndMenuBar();

        //print out all the messages
        for(int i = 0; i < this->LogMessages.size(); i++) {
            string sMessage = this->prefix + this->LogMessages[i].m;
            const char* cMessage = sMessage.c_str();
            //use different text color depending on level
            if(this->LogMessages[i].level == 0) { //set orange for event
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(227, 133, 31, 255)); //orange
            } else if(this->LogMessages[i].level == 1) { //set white for info
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255)); //white
            } else if(this->LogMessages[i].level == 2) { //set red for error message
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(179, 34, 18, 255)); //red

            }
            ImGui::TextWrapped(cMessage);
            ImGui::PopStyleColor(); //revert color change
        }
        ImGui::End();
        
        
    }

//methods for writing for log
//different methods so reworking the level system is easy
//Log info is for information presented without flavor
void Logger::LogInfo(string message) {
    logMessage n;
    n.level = 1;
    n.m = message;
    this->LogMessages.push_back(n);
}
//LogGameEvent is for the most essential messages, presented in the AI's "voice"
void Logger::LogGameEvent(string message) {
    logMessage n;
    n.level = 0;
    n.m = message;
    this->LogMessages.push_back(n);
}

//for non-fatal errors
void Logger::LogError(string message) {
    logMessage n;
    n.level = 2;
    n.m = message;
    this->LogMessages.push_back(n);
}