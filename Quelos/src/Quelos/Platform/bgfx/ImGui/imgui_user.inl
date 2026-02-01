namespace ImGui
{
    ImString::ImString()
        : Ptr(NULL)
    {
    }

    ImString::ImString(const ImString& rhs)
        : Ptr(NULL)
    {
        if (NULL != rhs.Ptr
        &&  0 != strcmp(rhs.Ptr, ""))
        {
            Ptr = ImStrdup(rhs.Ptr);
        }
    }

    ImString::ImString(const char* rhs)
        : Ptr(NULL)
    {
        if (NULL != rhs
        &&  0 != strcmp(rhs, ""))
        {
            Ptr = ImStrdup(rhs);
        }
    }

    ImString::~ImString()
    {
        Clear();
    }

    ImString& ImString::operator=(const ImString& rhs)
    {
        if (this != &rhs)
        {
            *this = rhs.Ptr;
        }

        return *this;
    }

    ImString& ImString::operator=(const char* rhs)
    {
        if (Ptr != rhs)
        {
            Clear();

            if (NULL != rhs
            &&  0 != strcmp(rhs, ""))
            {
                Ptr = ImStrdup(rhs);
            }
        }

        return *this;
    }

    void ImString::Clear()
    {
        if (NULL != Ptr)
        {
            MemFree(Ptr);
            Ptr = NULL;
        }
    }

    bool ImString::IsEmpty() const
    {
        return NULL == Ptr;
    }
} // namespace

#include "../../../ImGui/widgets/color_picker.inl"
#include "../../../ImGui/widgets/color_wheel.inl"
#include "../../../ImGui/widgets/dock.inl"
#include "../../../ImGui/widgets/file_list.inl"
#include "../../../ImGui/widgets/gizmo.inl"
#include "../../../ImGui/widgets/markdown.inl"
#include "../../../ImGui/widgets/range_slider.inl"
