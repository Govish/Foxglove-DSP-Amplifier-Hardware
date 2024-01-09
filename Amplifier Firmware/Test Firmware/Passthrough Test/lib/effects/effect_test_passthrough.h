
#include <effect_interface.h> //implements interface specified here
#include <scheduler.h> //to stage a transition 

//a simple category of effect that has no parameters and just sends audio through
//theme color is RED
class Effect_Test_Passthrough : public Effect_Interface {
public:
    //default constructor -- just call the base class constructor
    Effect_Test_Passthrough(RGB_LED::COLOR _theme_color, std::string _name);
    
    //implement the clone function to produce instances of this effect that are copies of the original
    //need to override base class so we produce copies of the derived class instead
    std::unique_ptr<Effect_Interface> clone() override;

    //provide implementations for the following functions:
    void audio_update(const Audio_Block_t& block_in, Audio_Block_t& block_out) override;
    std::string get_name() override;
    Effect_Icon_t get_icon() override;
    RGB_LED::COLOR get_theme_color() override;

private:
    //define implementation for `draw()` in the effect edit context
    //will just print "no params to adjust" centered on display
    void draw() override;

    //override the entry function, schedule a transition after one second
    void impl_on_entry() override;
    
    //have an icon for the effect, will be constant for all instances
    static const Effect_Icon_t icon;

    //have a particular name for our instance
    const std::string name; 

    //also have a theme color that can be configured instance-by-instance
    const RGB_LED::COLOR theme_color;    

    //use this to schedule a transition back to the previous page
    Scheduler done_editing_sched;
};