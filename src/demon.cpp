#include "../incl/demon.h"

Demon::Demon(const std::string &set_file): pid(nullptr),set_file(set_file)
{
    status = 0;
    if(this->ReadSetting(set_file.data()))WriteLog("Read all from setting file",log_file);
    else {
        std::cout<<"Can't find setting file\n"<<std::endl;
        exit(1);
}
    if(!readpath()){
        delete pid;
        exit(1);
    }
    WriteLog("Construct demon object",this->log_file);
}

Demon::~Demon()
{
    if(this->stop())WriteLog("Destroy Demon obj",log_file);
    if(pid!=nullptr)delete pid;
}

bool Demon::start()
{
///////////////////////////////////////////////////////////////////////////
    int               pid; 
    size_t exec_patc_count=0; 
    std::string exec_file; 

/////////////////////////////////////////////////////////////////////////

    while(input_user_path.size()>exec_patc_count)
  {//WILE_OPEN
    if (input_user_path.size()>exec_patc_count)
    {
        // создаём потомка
        pid = fork();
    }
    if (pid == -1) 
    {//FAIL_FORK        
        WriteLog(std::string("[MONITOR] Fork failed (%s)")+input_user_path[exec_patc_count],log_file);
        status = FAIL_FORK;
        if(status==FAIL_FORK){ 
            WriteLog(std::string("Fild open process with name: ")+input_user_path[exec_patc_count],log_file);
            ++exec_patc_count; exec_file = input_user_path[exec_patc_count];
            status = 0;
        }
    }//END_FAIL_FORK

    else if (!pid) 
    {
        if(execl(input_user_path[exec_patc_count].data(),"",NULL)==-1)
        {
         status = FAIL_EXEC;
         WriteLog(std::string("Faild exec process with name: ")+input_user_path[exec_patc_count],log_file);
         kill(getpid(),SIGTERM);
        }

    }
    else 
    {//PARENT_PROC        
        WriteLog(std::to_string(pid),log_file);
        pid_vec.push_back(pid);        
        ++exec_patc_count;
        if(input_user_path.size()>exec_patc_count)
        exec_file = input_user_path[exec_patc_count];
    }//END_PARENT_PROC
  }//END_WILE
    status=STR;
    return true;
}
//kill all another proc
bool Demon::stop()
{ 
    if(status==STR){
        sigignore(SIGCHLD);
        for(size_t i=0;i<pid_vec.size();++i){
            if(kill(pid_vec[i],SIGTERM)==-1){ 
                if(errno==ESRCH) {WriteLog("Error on kill proc",log_file);
                return false;}
            }//another errors
            else {WriteLog(std::string("Kill proc with pid: ")+std::to_string(pid_vec[i]),log_file);}
}

    pid_vec.clear();
    status = CLS;
    return true;
    }
    else return false;
}
//close dem, change cfg and rerun demon
bool Demon::restart()
{
if(stop())input_user_path.clear(); // clear path to proc from cfg file
if(readpath())
if(start())return true;
return false;
}
//Read all settings
bool Demon::ReadSetting(const char *FileName)
{
    std::string str;
    std::fstream in(FileName); // open file for reading
    if(!in.is_open()){       
        int fd = open(this->set_file.data(),O_CREAT|O_EXCL|O_TRUNC);
        if(fd<0)std::cout<<std::string("Can't create setting file\n")+set_file;
        else{
        const char* str("pid_file_path=\nconfig_file_path=\nlog_file_path=\n");
        write(fd,str,strlen(str));
        close(fd);
        }
        return false;
    }
   while(std::getline(in,str)) {
for(size_t i=0;i<str.size();++i){
    if(str[i]=='='){
       if(str.substr(0,i)=="pid_file_path") this->pid = new Pid_file(str.substr(i+1,str.size()-(i+2)));
       if(str.substr(0,i)=="config_file_path")         this->cfg_file=str.substr(i+1,str.size()-(i+2));
       if(str.substr(0,i)=="log_file_path")            this->log_file=str.substr(i+1,str.size()-(i+2));
    }

}
   }
    in.close();
    if(!cfg_file.empty()&&!log_file.empty())
    return true;
    else return false;
}
//read cfg file with path progs
bool Demon::readpath()
{

    std::string str;
    std::fstream in(this->cfg_file); // open file for reading

    if(!in.is_open()){
        WriteLog("Cant open cfg file",log_file);
        int fd = open(this->cfg_file.data(),O_CREAT|O_EXCL|S_IRWXO);
          if(fd<0){
              WriteLog("Error to create cfg file for user",log_file);
              return false;}
          else{
               WriteLog("Create cfg file with proc path",log_file);
               WriteLog(cfg_file.data(),log_file);
               WriteLog("Please write path to your porc there with (;) vat the end of each line",log_file);
               close(fd); return false;
          }
    }
   while(std::getline(in,str)) {

       ParsPath(str,input_user_path);
   }
   for(size_t i=0;i<input_user_path.size();++i) WriteLog(input_user_path[i],log_file);
           WriteLog("Read all from cfg file",log_file);

    in.close();
    return true;
}

bool Demon::del_pid(int pid)
{
    auto it = std::find(this->pid_vec.begin(),this->pid_vec.end(),pid);
    if (it != pid_vec.end()) {pid_vec.erase(it); return true;}
    else return false;
}

std::string Demon::getLog_file() const
{
    return log_file;
}
//main fun for demon proc
void MonitorProc(Demon &dem)
{
    int    status;
    sigset_t sigset;
    siginfo_t siginfo;


       sigemptyset(&sigset);
           sigaddset(&sigset, SIGQUIT);
           sigaddset(&sigset, SIGINT);
           sigaddset(&sigset, SIGTERM);
           sigaddset(&sigset, SIGCHLD);
           sigaddset(&sigset, SIGUSR1);
    
       sigprocmask(SIG_BLOCK, &sigset, NULL);


        if(dem.start())
          WriteLog("Dem start",dem.getLog_file());

while(true){
    sigwaitinfo(&sigset, &siginfo);
        if (siginfo.si_signo == SIGCHLD){ 
           pid_t chl_pid = wait(&status);
           if(dem.del_pid(chl_pid)) WriteLog("Close proc with pid: " + std::to_string(chl_pid), dem.getLog_file());           
        }
        else if(siginfo.si_signo==SIGUSR1){ 
            WriteLog("Restart",dem.getLog_file());
            dem.restart();
        }

        else if(siginfo.si_signo==SIGTERM||siginfo.si_signo==SIGINT||siginfo.si_signo==SIGQUIT)
        {
            dem.stop(); break;
    }
        }
return;
}
//Loging function on file "../Log/DemonLog.txt"
bool WriteLog(const std::string &msg, const std::string& Log_file="/home/user/Qt_prog/Log/DemonLog.txt")
{
    std::fstream* Logfile = new std::fstream(Log_file,std::fstream::in | std::fstream::out | std::fstream::app);
    if(!Logfile->is_open())
    {
        delete Logfile;
        int fd = open(Log_file.data(),O_CREAT|O_EXCL|O_TRUNC);
        if(fd<0) return false;
        else     return true;
        close(fd);
    }
    else{        
          time_t rawtime;
          struct tm * timeinfo;
          std::time( &rawtime );
          timeinfo = std::localtime ( &rawtime );
        *Logfile<<std::asctime (timeinfo);
        *Logfile<<"\t["<<msg<<"]"<<std::endl;
        Logfile->close();
        delete Logfile;
        return true;
    }    
    return false;
}
//Parsing line from cfg file
bool ParsPath(std::string str, std::vector<std::string>& vec_path)
{
    size_t j=0;
    for(size_t i=0;i<str.size();++i)
    {
        if((str[i]==';')&&(str[j]=='/'))
           {
            vec_path.push_back(static_cast<std::string>(str.substr(j,i-j)));return true;
           }
    }
    return false;
}
