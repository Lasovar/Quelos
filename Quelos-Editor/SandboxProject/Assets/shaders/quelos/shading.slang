import surface;
import brdf;
import ibl;

float3 ShadeSurface(SurfaceData s, float3 sunDirection, float3 sunRadiance, float environmentIntensity) {
    float3 color = EvaluateDirect(s, -sunDirection, sunRadiance);
    // TODO: Forward+: loop over this cluster's lights, calling EvaluateDirect for each
    color += EvaluateIBL(s) * environmentIntensity;

    return color;
}
