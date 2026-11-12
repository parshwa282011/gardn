#include <Client/Ui/InGame/GameInfo.hh>

#include <Client/Ui/Container.hh>
#include <Client/Ui/StaticText.hh>

#include <Client/DOM.hh>
#include <Client/Input.hh>
#include <Client/StaticData.hh>

#include <Client/Game.hh>

#include <string>

#include <iostream>

using namespace Ui;

static float const LEADERBOARD_WIDTH = 180;

LeaderboardSlot::LeaderboardSlot(uint8_t p) : Element(LEADERBOARD_WIDTH, 18) , pos(p) {
    ratio.set(1);
    style.animate = [&](Element *, Renderer &){
        if (pos >= Game::simulation.arena_info.player_count) {
            score = 0;
            return;
        }
        if (pos == LEADERBOARD_SIZE - 1 && Game::simulation.ent_alive(Game::player_id) &&
            Game::simulation.get_ent(Game::player_id).get_leaderboard_pos() == LEADERBOARD_SIZE) {
            score.set(Game::score);
            color = 0xff999999;
            name = &Game::simulation.get_ent(Game::player_id).get_name();
        } else {
            score.set(Game::simulation.arena_info.scores[pos]);
            if (Game::simulation.ent_alive(Game::player_id) &&
                Game::simulation.get_ent(Game::player_id).get_leaderboard_pos() == pos)
                color = 0xffffe763;
            else color = FLOWER_COLORS[Game::simulation.arena_info.colors[pos]];
            name = &Game::simulation.arena_info.names[pos];
        }
        score.step(Ui::lerp_amount);
        float r = 1;
        if (Game::simulation.arena_info.scores[0] != 0) 
            r = score.anchor() / Game::simulation.arena_info.scores[0];
        ratio.set(r);
        ratio.step(Ui::lerp_amount);
    };
}

void LeaderboardSlot::on_render(Renderer &ctx) {
    //we do not hide it though
    if (pos >= Game::simulation.arena_info.player_count) return;
    ctx.set_stroke(0xff222222);
    ctx.set_line_width(height);
    ctx.round_line_cap();
    ctx.begin_path();
    ctx.move_to(-(width-height)/2,0);
    ctx.line_to((width-height)/2,0);
    ctx.stroke();
    ctx.set_stroke(color);
    ctx.set_line_width(height * 0.8);
    ctx.begin_path();
    ctx.move_to(-(width-height)/2,0);
    ctx.line_to(-(width-height)/2+(width-height)*((float) ratio),0);
    ctx.stroke();
    std::string format_string = std::format("{} - {}", name_or_unnamed(*name), format_score((float) score));
    ctx.set_fill(0xffffffff);
    ctx.set_stroke(0xff222222);
    ctx.center_text_align();
    ctx.center_text_baseline();
    ctx.set_text_size(height * 0.75);
    ctx.set_line_width(height * 0.75 * 0.12);
    ctx.stroke_text(format_string.c_str());
    ctx.fill_text(format_string.c_str());
}

static Element *leaderboard_element = nullptr;

Element *Ui::make_color_legend() {
    Element *legend = new Ui::VContainer({
        new Ui::StaticText(18, "Colors")
    }, 12, 6, {
        .fill = 0xff555555,
        .line_width = 6,
        .round_radius = 7,
        .should_render = [](){
            return Game::should_render_game_ui() && !Game::show_chat && !DOM::text_input_focused() &&
                Input::keys_held.contains('\'');
        },
        .no_polling = 1
    });
    for (uint8_t i = 0; i < RarityID::kNumRarities; ++i)
        legend->add_child(new Ui::HContainer({
            new Ui::Element(18, 18, { .fill = RARITY_COLORS[i], .line_width = 3, .round_radius = 3 }),
            new Ui::StaticText(14, RARITY_NAMES[i])
        }, 0, 8, { .h_justify = Style::Left }));
    legend->style.h_justify = Style::Right;
    legend->style.v_justify = Style::Top;
    legend->x = -20;
    // right under the leaderboard, wherever its bottom is
    legend->style.animate = [](Element *elt, Renderer &) {
        elt->y = 20 + (leaderboard_element != nullptr ? leaderboard_element->height : 0) + 10;
    };
    return legend;
}

Element *Ui::make_leaderboard() {
    Container *lb_header = new Ui::Container({
        new Ui::DynamicText(18, [](){
            std::string format_string{"1 Flower"};
            if (Game::simulation.arena_info.player_count != 1) 
                format_string = std::format("{} Flowers", Game::simulation.arena_info.player_count);
            return format_string;
        })
    }, LEADERBOARD_WIDTH + 20, 48, { .fill = 0xff55bb55, .line_width = 6, .round_radius = 7 });

    Element *leaderboard = new Ui::VContainer({
        lb_header,
        new Ui::VContainer(
            Ui::make_range(0, LEADERBOARD_SIZE, [](uint32_t i){ return (Element *) (new Ui::LeaderboardSlot(i)); })
        , 10, 4, {})
    }, 0, 0, { 
        .fill = 0xff555555,
        .line_width = 6,
        .round_radius = 7,
        .should_render = [](){ return Game::should_render_game_ui(); },
        .no_polling = 1
    });
    leaderboard->style.h_justify = Style::Right;
    leaderboard->style.v_justify = Style::Top;
    leaderboard->x = -20;
    leaderboard->y = 20;
    leaderboard_element = leaderboard;
    return leaderboard;
}