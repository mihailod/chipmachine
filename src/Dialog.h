#ifndef DIALOG_H
#define DIALOG_H

#include "LineEdit.h"
#include "TextField.h"

class Dialog : public Renderable
{
public:

    Dialog(std::shared_ptr<grappix::RenderTarget> target,
           const grappix::Font& font, const std::string& text,
           float scale = 1.0F)
        : font(font), textField(font, text, 0, 0, scale),
          lineEdit(font, "", 0, 0, scale)
    {
        // Everything is expressed in terms of `scale`, so passing 1.5 makes the
        // whole dialog (box, label, input, cursor) 50% bigger.
        float pad = 24.0f * scale;
        float lineH = font.get_size("Ag", scale).y; // one text row's height
        float labelW = font.get_size(text, scale).x;

        // Box wide enough for the label, but at least half the screen so the
        // input has room. Two rows tall (label + input) with a roomier top margin
        // than the sides so the label does not crowd the upper border.
        float topPad = pad * 1.6f;
        // Clear separation between the label and the input. get_size() height
        // under-reports this display font's visual line height, so a small gap
        // lets the rows overlap -- key it off lineH plus padding and let the box
        // grow taller to fit.
        float gap = lineH * 1.5f + pad;
        bounds.w = std::max(labelW + pad * 2.0f, target->width() * 0.5f);
        bounds.h = topPad + lineH * 2.0f + gap + pad;

        bounds.x = (target->width() - bounds.w) / 2.0f;
        // Shifted upward a little so the box sits above dead centre.
        bounds.y = ((target->height() - bounds.h) / 2.0f) - 50.0f;

        textField.pos = { bounds.x + pad, bounds.y + topPad };
        lineEdit.pos = { bounds.x + pad, bounds.y + topPad + lineH + gap };

        // Cap the name at however many of the WIDEST glyph fit across the input
        // area, so typed text can never spill past the box edge regardless of
        // which characters are used (see on_key). At least one.
        float charW = font.get_size("W", scale).x;
        maxLen = charW > 0 ? std::max(1, (int)((bounds.w - pad * 2.0f) / charW))
                           : 1;
    }

    // onOk returns true to accept the input and close the dialog, false to
    // reject it (bad/duplicate name) and keep the dialog open so the user can
    // correct it. A dialog with no callback always closes on ENTER.
    void on_ok(std::function<bool(const std::string&)> cb)
    {
        onOk = cb;
    }

    void on_key(uint32_t key)
    {
        LOGD("DIALOG: %d", key);
        if (key == keycodes::ENTER) {
            if (!onOk || onOk(lineEdit.getText()))
                Renderable::remove();
        } else if (key == keycodes::ESCAPE) {
            Renderable::remove();
        } else {
            // Swallow further printable input once the name is full; editing keys
            // (BACKSPACE / cursor) still pass through so the field stays usable.
            bool printable = (key >= 0x20 && key < 0x80);
            if (printable && lineEdit.getText().size() >= (size_t)maxLen) return;
            lineEdit.on_key(key);
        }
    }

    virtual void render(std::shared_ptr<grappix::RenderTarget> target,
                        uint32_t delta) override
    {
        target->rectangle(bounds, 0x80ffffff);
        textField.render(target, delta);
        lineEdit.render(target, delta);
    }

    std::function<bool(const std::string&)> onOk;
    grappix::Font font;
    std::string text;
    grappix::Rectangle bounds;
    TextField textField;
    LineEdit lineEdit;
    int maxLen = 1; // longest name that fits the box (set in the constructor)
};

#endif // DIALOG_H
