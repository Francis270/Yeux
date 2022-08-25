#include <iostream>
#include <Yeux.hpp>

int main()
{
    yeux::Yeux yeuxobj;

    std::cout << "Yeux Version: " << yeuxobj.version() << std::endl;
    yeuxobj.setup("fra");// eng
    
    yeuxobj.screenshot(0, 0, 1920, 1080, "tmp");// make current screenshot
    
    // can call getText, getColor and getRGB as many times on current screenshot
    yeuxobj.getText("fichier", 547, 7, 587, 24, 2.0, "", true);
    yeuxobj.getRGB("fichier", 0, 0, true);
    
    yeuxobj.clean();// clean after done with the current screenshot
}
