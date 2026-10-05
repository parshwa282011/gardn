#pragma once

#include <Client/Ui/Element.hh>
#include <Client/Ui/Container.hh>

#include <Shared/StaticData.hh>

namespace Ui {
    class StatPetalSlot final : public Element {
    public:
        uint8_t pos;
        StatPetalSlot(uint8_t);

        virtual void on_render(Renderer &) override;

        virtual void on_event(uint8_t) override;
    };

    class DeadFlowerIcon final : public Element {
    public:
        DeadFlowerIcon(float);

        virtual void on_render(Renderer &) override;
    };

    class TitlePetalSlot final : public Element {
    public:
        uint8_t pos;
        TitlePetalSlot(uint8_t);

        virtual void on_render(Renderer &) override;

        virtual void on_event(uint8_t) override;
    };

    class GalleryPetal final : public Element {
    public:
        PetalID::T id;
        GalleryPetal(PetalID::T, float);

        virtual void on_render(Renderer &) override;
        virtual void on_event(uint8_t) override;
    };

    class PetalsCollectedIndicator final : public Element {
    public:
        PetalsCollectedIndicator(float);

        virtual void on_render(Renderer &) override;
    };

    class GalleryMob final : public Element {
    public:
        MobID::T id;
        GalleryMob(MobID::T, float);

        virtual void on_render(Renderer &) override;
    };

    class ProgressPetalSlot final : public Element {
    public:
        uint32_t idx;
        uint8_t pos;
        ProgressPetalSlot(uint32_t, uint8_t, float);

        virtual void on_render(Renderer &) override;
        virtual void on_event(uint8_t) override;
    };

    class DynamicProgressSaves final : public VContainer {
    public:
        DynamicProgressSaves();

        virtual void refactor() override;
    };

    Element *make_title_input_box();
    Element *make_title_info_box();
    Element *make_panel_buttons();
    Element *make_death_main_screen();
    Element *make_stat_screen();
    Element *make_settings_panel();
    Element *make_petal_gallery();
    Element *make_mob_gallery();
    Element *make_progress_saves();
    Element *make_changelog();
    Element *make_debug_stats();
    Element *make_link_buttons();
}