#include <Client/Ui/InGame/GameInfo.hh>

#include <Client/Ui/Container.hh>
#include <Client/Ui/StaticText.hh>

#include <Client/DOM.hh>
#include <Client/Input.hh>
#include <Client/StaticData.hh>

#include <Client/Game.hh>

#include <algorithm>
#include <string>
#include <vector>

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

// What the colors of the dots of the observer mean: the mobs picked in the mob gallery,
// each with its color. It draws its own box from the top, which grows with the list.
class ObserverLegend final : public Ui::Element {
public:
    ObserverLegend() : Element(420, 0, {
        .should_render = [](){
            return Game::should_render_game_ui() && !Game::show_chat && !DOM::text_input_focused() &&
                Input::keys_held.contains(Input::QuoteKey);
        },
        .h_justify = Style::Right,
        .v_justify = Style::Top,
        .no_polling = 1
    }) {}

    virtual void on_render(Renderer &ctx) override {
        std::vector<MobID::T> ids;
        for (MobID::T id = 0; id < MobID::kNumMobs; ++id)
            if (Game::tracked_mobs[id]) ids.push_back(id);
        float const col_w = 190;
        float const row_h = 24;
        float const pad = 12;
        float const head_h = 30;
        uint32_t const per_col = 12;
        uint32_t cols = ids.size() > per_col ? 2 : 1;
        uint32_t rows = ids.empty() ? 1 : std::min<uint32_t>(ids.size(), per_col);
        float box_w = pad * 2 + cols * col_w;
        float box_h = pad * 2 + head_h + rows * row_h;
        // the box hangs from the top right of the element
        ctx.translate(width / 2 - box_w / 2, 0);
        ctx.set_fill(0xff555555);
        ctx.set_stroke(Renderer::HSV(0xff555555, 0.8));
        ctx.set_line_width(6);
        ctx.round_line_join();
        ctx.begin_path();
        ctx.round_rect(-box_w / 2, 0, box_w, box_h, 7);
        ctx.fill();
        ctx.stroke();
        {
            RenderContext context(&ctx);
            ctx.translate(0, pad + head_h / 2);
            ctx.draw_text("Observer dots", { .size = 18 });
        }
        if (ids.empty()) {
            RenderContext context(&ctx);
            ctx.translate(0, pad + head_h + row_h / 2);
            ctx.draw_text("Pick mobs in the Mob Gallery", { .size = 14 });
            return;
        }
        for (uint32_t i = 0; i < ids.size(); ++i) {
            RenderContext context(&ctx);
            float left = -box_w / 2 + pad + (i / per_col) * col_w;
            ctx.translate(left, pad + head_h + (i % per_col) * row_h + row_h / 2);
            ctx.set_fill(mob_track_color(ids[i]));
            ctx.set_stroke(0xff222222);
            ctx.set_line_width(2.5);
            ctx.begin_path();
            ctx.arc(8, 0, 8);
            ctx.fill();
            ctx.stroke();
            char const *name = MOB_DATA[ids[i]].name;
            float text_w = Renderer::get_ascii_text_size(name) * 14;
            ctx.translate(8 * 2 + 8 + text_w / 2, 0);
            ctx.draw_text(name, { .size = 14 });
        }
    }
};

static Element *leaderboard_element = nullptr;

Element *Ui::make_color_legend() {
    Element *legend = new ObserverLegend();
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