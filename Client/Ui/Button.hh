#pragma once

#include <Client/Ui/Element.hh>

namespace Ui {
    class Button : public Element {
        std::function<void (Element *, uint8_t)> on_click;
        bool (*should_darken)();
    public:
        Button(float, float, Element *, std::function<void (Element *, uint8_t)> = [](Element *, uint8_t){}, bool (void) = nullptr, Style = { .fill = 0xffffffff, .stroke_hsv = 0.8 });

        virtual void on_render(Renderer &) override;
        virtual void refactor() override;
        virtual void on_event(uint8_t) override;
    };

    class ShakeButton : public Button {
        bool (*should_shake)();
        bool is_shaking = false;
    public:
        ShakeButton(float, float, Element *, void (Element *, uint8_t) = [](Element *, uint8_t){}, bool (void) = nullptr, bool (void) = nullptr, Style = { .fill = 0xffffffff, .stroke_hsv = 0.8 });

        virtual void on_render(Renderer &) override;
    };

    class ToggleButton : public Element {
        uint8_t *toggler;
        float lerp_toggle;
    public:
        ToggleButton(float, uint8_t *);

        virtual void on_render(Renderer &) override;
        virtual void on_event(uint8_t) override;
    };
}