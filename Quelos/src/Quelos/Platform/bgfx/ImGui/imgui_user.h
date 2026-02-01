#include <stdint.h>
#include <inttypes.h>

namespace ImGui
{
    struct Font
    {
        enum Enum
        {
            Regular,
            Mono,

            Count
        };
    };

    void PushFont(Font::Enum _font, float _fontSizeBaseUnscaled = 0.0f);

    // BK - simple string class for convenience.
    class ImString
    {
    public:
        ImString();
        ImString(const ImString& rhs);
        ImString(const char* rhs);
        ~ImString();

        ImString& operator=(const ImString& rhs);
        ImString& operator=(const char* rhs);

        void Clear();
        bool IsEmpty() const;

        const char* CStr() const
        {
            return NULL == Ptr ? "" : Ptr;
        }

    private:
        char* Ptr;
    };

} // namespace ImGui

#include "../../../ImGui/widgets/color_picker.h"
#include "../../../ImGui/widgets/color_wheel.h"
#include "../../../ImGui/widgets/dock.h"
#include "../../../ImGui/widgets/file_list.h"
#include "../../../ImGui/widgets/gizmo.h"
#include "../../../ImGui/widgets/markdown.h"
#include "../../../ImGui/widgets/range_slider.h"
