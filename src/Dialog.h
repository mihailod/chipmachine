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
    : font(font), textField(font, text), lineEdit(font)
{
    auto size = font.get_size(text, scale);
    
    float minW = 150.0f; // Increased minimum width for better padding
    float minH = 80.0f;  // Increased minimum height
    
    bounds.w = std::max(size.x + 40.0f, minW);
    bounds.h = std::max(size.y * 4.0f, minH);
    
    bounds.x = (target->width() - bounds.w) / 2.0f;
    bounds.y = (target->height() - bounds.h) / 2.0f;
    
    // Position children relative to the bounds origin
    textField.pos = { bounds.x + 20.0f, bounds.y + 10.0f };
    lineEdit.pos  = { bounds.x + 20.0f, bounds.y + (bounds.h / 2.0f) };
}

    void on_ok(std::function<void(const std::string&)> cb)
    {
        // lineEdit.on_ok(cb);
        onOk = cb;
    }

    void on_key(uint32_t key)
    {

        LOGD("DIALOG: %d", key);

        if (key == keycodes::ENTER) {
            if (onOk) onOk(lineEdit.getText());
            Renderable::remove();
        } else if (key == keycodes::ESCAPE) {
            Renderable::remove();
        } else {
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

    std::function<void(const std::string&)> onOk;

    grappix::Font font;
    std::string text;

    grappix::Rectangle bounds;
    TextField textField;
    LineEdit lineEdit;
};

#endif // DIALOG_H
