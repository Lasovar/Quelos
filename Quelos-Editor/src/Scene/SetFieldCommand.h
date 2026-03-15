#pragma once

#include <string_view>
#include <vector>

#include "Quelos/Scenes/ComponentReference.h"

namespace Quelos {
    class SetFieldArchive {
    public:
        static constexpr bool IsLoading = true;
        static constexpr bool IsSaving = false;

    public:
        explicit SetFieldArchive(const std::string_view fieldName, void* value)
            : m_FieldKey(fieldName), m_Value(value) {}

        template <typename T>
        void Field(const std::string_view name, T& value) {
            if (name == m_FieldKey) {
                value = *static_cast<T*>(m_Value);
            }
        }

        // TODO:
        template <typename T>
            requires(std::is_trivially_copyable_v<T>)
        void FieldVector(const std::string&, std::vector<T>& data) { }
        template <typename T>
        void Value(T& value) { }
        static void BeginTuple() { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }

    private:
        std::string_view m_FieldKey;
        void* m_Value;
    };

    using SetFieldSerializeFn = void(*)(SetFieldArchive&, void*);

    template <typename TField>
    struct SetField {
        ComponentUntypedRef ComponentRef;
        SetFieldSerializeFn SerializeComponentFunc = nullptr;

        std::string_view FieldKey;

        TField Before;
        TField After;

        void Apply() {
            SetFieldArchive archive(FieldKey, &After);
            SerializeComponentFunc(archive, ComponentRef.Get());
        }

        void Revert() {
            auto archive = SetFieldArchive(FieldKey, &Before);
            SerializeComponentFunc(archive, ComponentRef.Get());
        }
    };
}
