#pragma once

#include <Client/Ui/Element.hh>

#include <Shared/StaticData.hh>

namespace Ui {
    // a clickable row of the petal list of the dev menu: the petal, in its rarity color
    class DevPetalRow final : public Element {
    public:
        PetalID::T id;
        DevPetalRow(PetalID::T);

        virtual void on_render(Renderer &) override;
        virtual void on_event(uint8_t) override;
    };

    // a clickable row of the mob list of the dev menu
    class DevMobRow final : public Element {
    public:
        MobID::T id;
        DevMobRow(MobID::T);

        virtual void on_render(Renderer &) override;
        virtual void on_event(uint8_t) override;
    };

    // The developer menu (the \ key). It only exists in the sandbox, for everybody who
    // plays in it: set the level, get petals, summon mobs, clear them, be invincible.
    Element *make_dev_menu();
    // whether the mouse is over the menu (the clicks and the movement are not for the
    // flower then)
    bool dev_menu_contains_mouse();
}
