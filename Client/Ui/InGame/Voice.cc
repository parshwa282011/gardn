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
            new Ui::StaticIcon([](Element *elt, Renderer &ctx){
                // a microphone, drawn in a 24x24 box around the middle of the button
                bool off = Game::voice_wanted == VoiceState::kOff;
                bool talking = Game::voice_wanted == VoiceState::kTalking;
                ctx.scale(elt->width / 24);
                ctx.round_line_cap();
                ctx.round_line_join();
                uint32_t ink = off ? 0xffe4e4e4 : 0xffffffff;
                ctx.set_fill(ink);
                ctx.set_stroke(ink);
                // the capsule
                ctx.begin_path();
                ctx.round_rect(-3.4, -10, 6.8, 12.5, 3.4);
                ctx.fill();
                // the cradle around it, the stem and the foot
                ctx.set_line_width(2);
                ctx.begin_path();
                ctx.move_to(-6.6, -1.5);
                ctx.bcurve_to(-6.6, 6, 6.6, 6, 6.6, -1.5);
                ctx.stroke();
                ctx.begin_path();
                ctx.move_to(0, 4.6);
                ctx.line_to(0, 9.4);
                ctx.move_to(-3.8, 9.6);
                ctx.line_to(3.8, 9.6);
                ctx.stroke();
                if (talking) {
                    // sound waves on both sides while there is sound
                    ctx.set_line_width(1.6);
                    ctx.begin_path();
                    ctx.move_to(9.6, -5.5);
                    ctx.bcurve_to(11.6, -3, 11.6, 1, 9.6, 3.5);
                    ctx.move_to(-9.6, -5.5);
                    ctx.bcurve_to(-11.6, -3, -11.6, 1, -9.6, 3.5);
                    ctx.stroke();
                }
                if (off) {
                    // crossed out: a dark line with a red one on top
                    ctx.set_line_width(4.2);
                    ctx.set_stroke(0xff3a1414);
                    ctx.begin_path();
                    ctx.move_to(-8.5, -10.5);
                    ctx.line_to(8.5, 10.5);
                    ctx.stroke();
                    ctx.set_line_width(2.4);
                    ctx.set_stroke(0xffff5959);
                    ctx.begin_path();
                    ctx.move_to(-8.5, -10.5);
                    ctx.line_to(8.5, 10.5);
                    ctx.stroke();
                }
            }, 30, 30),
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
