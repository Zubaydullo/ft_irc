






#include "./Server/Server.hpp"





int main(int ac , char **av){

        if(ac != 3 )
        {
            std::cerr << "Usage : " << av[0] << "  <port> <password>" <<std::endl;
            return 0;
        }
        
        // -------init the port & password--- //
        int port = std::atoi(av[1]);
        std::string password = av[2];
        //---------- end -------------------//
       
    if(port <= 0  ||  port > 65535)
     {
         std::cerr << "Error: Invalid Port Number"  << std::endl;
        return (1);
     }
    if(password.empty()){
         std::cerr << "Error : The password empty" << std::endl;
         return (1);
    }

         std::cout << "Starting IRC Server on port :  " << port  <<std::endl;
    
   try{ 
        Server server(port, password);
        server.Start();
   }
   catch(const std::exception&  e){
       std::cerr << "Error : " << e.what()  << std::endl;
        return (1);
   }

    return(0);
}
