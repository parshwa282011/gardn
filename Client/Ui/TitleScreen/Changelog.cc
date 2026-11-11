#include <Client/Ui/TitleScreen/TitleScreen.hh>

#include <Client/Ui/Container.hh>
#include <Client/Ui/Extern.hh>
#include <Client/Ui/StaticText.hh>

#include <Client/Game.hh>

#include <emscripten/wget.h>

#include <string>
#include <vector>

using namespace Ui;

// The top of the list is not in the client: it is /changelog.txt (rrolf/Client/public,
// the same file Parteria reads), so a change that is the same in both games is written
// once. The format is described at the top of the file. This game shows the days with
// [all] and [gardn] lines. Below them are the older entries, which are in the code.
struct GeneralDay {
    std::string date;
    std::vector<std::string> items;
};
static std::vector<GeneralDay> general;
static bool general_dirty = false;

static Element *make_divider() {
    return new Element(400, 6, { .fill = 0x20000000, .line_width = 3, .round_radius = 3, .v_justify = Style::Top });
}

static Element *make_date(std::string const date) {
    return new Ui::StaticText(20, date, { .fill = 0xffffffff, .h_justify = Style::Left, .v_justify = Style::Top });
}

static Element *make_paragraph(std::string const contents) {
    return new Ui::HContainer({
        new Ui::StaticText(14, "-", { .fill = 0xffffffff, .v_justify = Style::Top }),
        new Ui::StaticParagraph(360, 14, contents)
    }, 0, 10, { .h_justify = Style::Left, .v_justify = Style::Top });
}

static Element *make_entries(std::initializer_list<std::string const> contents) {
    Game::changelog_entries += contents.size();
    Element *elt = new Ui::VContainer({}, 5, 3, { .h_justify = Style::Left, .v_justify = Style::Top });
    for (std::string const str : contents)
        elt->add_child(make_paragraph(str));
    return elt;
}

static Element *make_listed_entries(std::vector<std::string> const &contents) {
    Element *elt = new Ui::VContainer({}, 5, 3, { .h_justify = Style::Left, .v_justify = Style::Top });
    for (std::string const &str : contents)
        elt->add_child(make_paragraph(str));
    return elt;
}

// the days of changelog.txt, rebuilt when the file arrives
class GeneralSection final : public Ui::VContainer {
public:
    GeneralSection() : VContainer({}, 0, 10, { .h_justify = Style::Left, .v_justify = Style::Top }) {}

    virtual void refactor() override {
        if (general_dirty) {
            general_dirty = false;
            children.clear();
            for (GeneralDay const &day : general) {
                Element *date = make_date(day.date);
                Element *items = make_listed_entries(day.items);
                Element *divider = make_divider();
                for (Element *elt : {date, items, divider}) {
                    elt->style.v_justify = Style::Top;
                    add_child(elt);
                }
            }
        }
        VContainer::refactor();
    }
};

static void parse_general(std::string const &text) {
    general.clear();
    uint32_t items = 0;
    size_t pos = 0;
    GeneralDay *day = nullptr;
    std::string pending_date;
    while (pos < text.size()) {
        size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(pos, end - pos);
        pos = end + 1;
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("== ", 0) == 0) {
            pending_date = line.substr(3);
            day = nullptr;
            continue;
        }
        if (line[0] != '[') continue;
        size_t close = line.find(']');
        if (close == std::string::npos) continue;
        std::string tag = line.substr(1, close - 1);
        if (tag != "all" && tag != "gardn") continue;
        std::string item = line.substr(close + 1);
        while (!item.empty() && item[0] == ' ') item.erase(0, 1);
        if (item.empty()) continue;
        // the day is added when its first change for this game comes
        if (day == nullptr) {
            general.push_back({ pending_date, {} });
            day = &general.back();
        }
        day->items.push_back(item);
        ++items;
    }
    // the entries are what the Changelog button counts to shake when there is news
    Game::changelog_entries += items;
    general_dirty = true;
}

static void general_loaded(void *, void *buffer, int size) {
    parse_general(std::string(static_cast<char const *>(buffer), size));
}

static void general_failed(void *) {}

static Element *make_changelog_contents() {
    Element *elt = new Ui::VContainer({
        new GeneralSection(),
        make_date("October 5th 2026"),
        make_entries({
            "Automated progress save and restore system",
            "Note that progress save IDs are stored locally and will be gone if you clear site data",
            "Restoring is disabled while current progress is eligible for saving, to prevent overwriting it",
            "Server-side persistent storage is still in development: keeping additional proof of your progress won't hurt"
        }),
        make_divider(),
        make_date("December 14th 2025"),
        make_entries({
            "Added 8 new petals",
            "Added new zone",
            "Reworked Yggdrasil",
            "Nerfed drop rate by 2x",
            "Nerfed eggs",
            "Other balancing"
        }),
        make_divider(),
        make_date("December 3rd 2025"),
        make_entries({
            "Added game mode switch",
            "Switching the mode preserves your level and petals"
        }),
        make_divider(),
        make_date("November 30th 2025"),
        make_entries({
            "Reworked Yggdrasil"
        }),
        make_divider(),
        make_date("November 29th 2025"),
        make_entries({
            "Reworked Yggdrasil"
        }),
        make_divider(),
        make_date("November 8th 2025"),
        make_entries({
            "Added 3 new petals",
            "Added 2 new mobs",
            "Added leader curses at levels 75 and 90",
            "Added auto-delete drops setting",
            "Buffed drop rate by 2x",
            "Buffed mob density",
            "Other balancing"
        }),
        make_divider(),
        make_date("October 25th 2025"),
        make_entries({
            "Added 1 new petal",
            "Peas now split into 4 orthogonal directions",
            "Even more bugfixes"
        }),
        make_divider(),
        make_date("October 20th 2025"),
        make_entries({
            "Added 4 new petals"
        }),
        make_divider(),
        make_date("October 6th 2025"),
        make_entries({
            "Added a button to force update to the latest game version"
        }),
        make_divider(),
        make_date("September 27th 2025"),
        make_entries({
            "Game updates are now seamless for players",
            "Several servers may exist at the same time"
        }),
        make_divider(),
        make_date("September 14th 2025"),
        make_entries({
            "Added more mobile buttons",
            "Fixed nickname input on mobile"
        }),
        make_divider(),
        make_date("September 11th 2025"),
        make_entries({
            "Players can now rejoin the game for up to 1 minute after leaving"
        }),
        make_divider(),
        make_date("September 3rd 2025"),
        make_entries({
            "Buffed Bone",
            "Teammates are now displayed on the minimap"
        }),
        make_divider(),
        make_date("August 30th 2025"),
        make_entries({
            "Added mobile touch controls",
            "Enabled collision between friendly entities",
            "Neutral mobs will now aggro the leader if above level 60",
            "More bugfixes"
        }),
        make_divider(),
        make_date("August 25th 2025"),
        make_entries({
            "Buffed Peas",
            "Reworked Yggdrasil",
            "Rose and Dahlia can now heal teammates (including Diggers!)"
        }),
        make_divider(),
        make_date("August 20th 2025"),
        make_entries({
            "Added chat"
        }),
        make_divider(),
        make_date("August 14th 2025"),
        make_entries({
            "Added ability to manually restore score and petals for players who leave the game without dying"
        }),
        make_divider(),
        make_date("August 10th 2025"),
        make_entries({
            "Added TDM",
            "Made some faster mobs slower",
            "Various bugfixes"
        }),
        make_divider(),
        make_date("August 4th 2025"),
        make_entries({
            "Increased max slot number to 12"
        }),
        make_divider(),
        make_date("July 31st 2025"),
        make_entries({
            "Added 1 new petal",
            "Changed initial petal spawn delay from 2.5s to 1s"
        }),
        make_divider(),
        make_date("June 15th 2025"),
        make_entries({
            "Finished base game",
            "Added 3 new petals",
            "Drop rates are now autobalanced",
            "Spawning a petal for the first time now takes 2.5 extra seconds"
        }),
        make_divider(),
        make_date("June 11th 2025"),
        make_entries({
            "Added 1 new mob",
            "Balanced some mob stats",
            "Revamped petal descriptions"
        }),
        make_divider(),
        make_date("June 7th 2025"),
        make_entries({
            "Finished changelog",
            "Tweaked some petal stats",
            "Added settings and mob gallery"
        }),
        make_divider(),
        make_date("June 6th 2025"),
        make_entries({
            "Added Changelog",
            "Added 2 new petals",
            "Added 1 new mob",
        }),
        make_divider(),
        make_date("March 23rd 2025"),
        make_entries({
            "Game is now playable",
            "Added spawner petals"
        }),
        make_divider(),
        new Ui::StaticText(14, "Older changelog entries not available")
    }, 0, 10, {});
    return new Ui::ScrollContainer(elt, 300);
}

Element *Ui::make_changelog() {
    Element *elt = new Ui::VContainer({
        new Ui::StaticText(25, "Changelog"),
        make_changelog_contents()
    }, 15, 10, { 
        .fill = 0xff5a9fdb,
        .line_width = 7,
        .round_radius = 3,
        .animate = [](Element *elt, Renderer &ctx){
            ctx.translate(0, (1 - elt->animation) * 2 * elt->height);
        },
        .should_render = [](){
            return Ui::panel_open == Panel::kChangelog && Game::should_render_title_ui();
        },
        .h_justify = Style::Left,
        .v_justify = Style::Bottom
    });
    Ui::Panel::changelog = elt;
    emscripten_async_wget_data("/changelog.txt", nullptr, general_loaded, general_failed);
    return elt;
}