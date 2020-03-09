#include <stdio.h>

namespace
{
    bool logging = false;

    FILE* OpenLog()
    {
        FILE*	f;
        
        f = fopen("VERGE.LOG", "a");
        //    if (!f)
        //        Sys_Error("OpenLog: unable to open VERGE.LOG");
        
        return	f;
    }

}

namespace Log
{
    
    void Enable()
    {
        logging = true;
    }
    
    void Init()
    {
        if (logging)
        {
            remove("verge.log");
        }
    }
    
    
    void Write(const char* message)
    {
        FILE*	f;
        
        printf("%s\n", message);
        
        if (!logging)
            return;
        
        f = OpenLog();
        
        //    if (!f) Sys_Error("Error logging!");
        
        fprintf(f, "%s\n", message);
        fflush(f);
        
        fclose(f);
    }
    
    void Writen(const char* message)
    {
        FILE*	f;
        
        printf("%s", message);
        
        if (!logging)
            return;
        
        f = OpenLog();
        
        //    if (!f) Sys_Error("Error logging!");
        
        fprintf(f, "%s", message);
        fflush(f);
        
        fclose(f);
    }
        
};