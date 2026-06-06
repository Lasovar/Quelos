import quelos;

__extern_cpp struct EditorInstanceData {
    float4x4 Transform;
    uint EntityID;
};

__extern_cpp StructuredBuffer<EditorInstanceData> EditorInstances;
