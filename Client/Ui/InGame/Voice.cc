#include <Client/Ui/InGame/GameInfo.hh>

#include <Client/Ui/Button.hh>
#include <Client/Ui/Container.hh>
#include <Client/Ui/DynamicText.hh>
#include <Client/Ui/StaticText.hh>

#include <Client/DOM.hh>
#include <Client/Game.hh>

using namespace Ui;

// Top left, under the sandbox line: the mic of the proximity voice chat (the V key does the
// same). Players who have it on hear each other when they are close, louder the closer.
Element *Ui::make_voice_button() {
    Element *elt = new Ui::HContainer({
        new Ui::Button(44, 44,
            new Ui::StaticText(14, "Mic"),
            [](Element *, uint8_t e){
                if (e == Ui::kClick) DOM::voice_toggle();
            },
            [](){ return Game::voice_wanted != VoiceState::kOff; },
            { .fill = 0xff3fbf6f, .line_width = 5, .round_radius = 5 }
        ),
        new Ui::DynamicText(15, [](){
            switch (Game::voice_wanted) {
                case VoiceState::kMicOpen: return std::string{"Voice chat on (V to turn off). Nearby players with it on hear you"};
                case VoiceState::kTalking: return std::string{"Talking..."};
                default: return std::string{"Press V for proximity voice chat"};
            }
        }, { .fill = 0xffffffff, .h_justify = Style::Left })
    }, 10, 10, {
        .should_render = [](){ return Game::alive(); },
        .h_justify = Style::Left,
        .v_justify = Style::Top,
        .no_animation = 1
    });
    elt->y = 110;
    return elt;
}
