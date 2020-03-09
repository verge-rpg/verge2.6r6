#ifndef CONSOLE_H
#define CONSOLE_H

#include "vtypes.h"
#include <vector>
#include <map>
#include <list>
#include <string>

class Console
{
    typedef std::vector<std::string> StringVec;
    typedef void (Console::*ConsoleFunc)(const StringVec& args);
    typedef std::list<std::string> StringList;
    typedef std::map<std::string, ConsoleFunc> FunctionMap;

    // Temporary, until a proper image infrastructure is put in place.
    struct Image
    {
        int width;
        int height;
        u8* pixels;

        Image()
        {
            width = height = 0;
            pixels = 0;
        }
    };

    Image background;

    int yposition;  // 0 is completely hidden from view.  gfx.YRes()/2 is all the way down
    int direction;  // -1 = moving up.  0 = not moving.  1 = moving down
    int hFont;      // font handle

    FunctionMap functions;  // functions the console knows about
    StringList  output;     // text output
    std::string curcommand; // current text buffer
    bool        enabled;

public:
    Console();
    ~Console();

    void Draw();
    void SendKey(char c);
    void Exec(const std::string& command);

    void Open();
    void Close();
    bool IsOpen();

    void Enable();
    void Disable();

    void Write(const char* msg);

private:
    std::string TabComplete(const std::string& cmd);

    // Console functions
    void Ver(const StringVec& args);
    void Clear(const StringVec& args);
    void Exit(const StringVec& args);
    void Help(const StringVec& args);
    void SetBackground(const StringVec& args);
    void CPUInfo(const StringVec& args);
};

#endif
