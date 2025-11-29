// Максимальное значение для float
#define FLT_MAX 3.402823466e+38F
#define TYPE_SPHERE 0
#define TYPE_PLANE 1

RWTexture2D<float4> OutputTexture : register(u0);

struct HitInfo
{
    float t;
    float3 normal;
};

cbuffer SceneConstants : register(b0)
{
    float3 camera_center;
    float _pad0;
    
    float3 pixel00_loc;
    float _pad1;
    
    float3 pixel_delta_u;
    float _pad2;
    
    float3 pixel_delta_v;
    float _pad3;
        
    uint primitiveCount;
    uint trianglesCount;
    uint sampleCount;
    float _pad5;
    
    float3 enviroment;
    float frameTime;    
};

struct Primitive
{
    uint type;
    uint matIndex;
    float radius;
    float _pad0;
    
    float4 position_point;
};

struct Triangle
{
    float3 a;
    float _padA;
    float3 b;
    float _padB;
    float3 c;
    float _padC;
    uint matIndex;
    float3 _pad;
};

struct Material
{
    float3 albedo;
    float _pada;
    float3 emmision;
    float _pade;
    uint type;
    float _pad[3];
};

struct Ray
{
    float3 origin;
    float3 direction;
};

static const float PI = 3.14159265359;

StructuredBuffer<Primitive> ScenePrimitives : register( t0 );
StructuredBuffer<Triangle> SceneTriangles   : register( t1 );
StructuredBuffer<Material> SceneMaterials   : register( t2 );


float RandomFloat(float2 p, float seed)
{
    float2 temp = p + seed;
    float n = dot(temp, float2(12.9898, 78.233));
    return frac(sin(n) * 43758.5453123);
}

float3 randomUniformVectorHemispher(float2 screanPos)
{
    float randOne = RandomFloat(screanPos, frameTime);
    float randTwo = RandomFloat(screanPos + 0.1 , frameTime);
    float phi = randOne * 2.0 * PI;
    float cosTheta = randTwo * 2.0 - 1.0;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    float x = cos(phi) * sinTheta;
    float y = cosTheta;
    float z = sin(phi) * sinTheta;
    return float3(x, y, z);
}

// Функция для трассировки луча со сферой (Оптимизирована: a = 1.0)
HitInfo IntersectSphere(Ray ray, float3 sphereCenter, float sphereRadius)
{
    HitInfo hit = { FLT_MAX, float3(0, 0, 0)};

    // Вектор от начала луча до центра сферы: oc = A - C
    float3 oc = ray.origin - sphereCenter;

    // Квадратное уравнение: at^2 + bt + c = 0
    // Так как rayDirection нормализован, a = 1.0. 
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;

    // Дискриминант: D = b^2 - 4ac. Т.к. a=1, то D = b^2 - 4c
    float discriminant = b * b - 4.0 * c;
        
    // Если дискриминант >= 0, есть пересечение
    if (discriminant >= 0)
    {
        // t = (-b - sqrt(D)) / 2a. Т.к. a=1, делим на 2.0
        float t = (-b - sqrt(discriminant)) / 2.0;

        // Использование небольшого смещения t > 0.001 для устранения артефактов
        if (t > 0.001)
        {
            hit.t = t;
            
            float3 hitPoint = ray.origin + ray.direction* t;
            hit.normal = normalize(hitPoint - sphereCenter);
            return hit;
        }
    }

    return hit;
}


HitInfo IntersectPlane( float3 rayOrigin, float3 rayDirection, float3 pointOnPlane, float3 normalPlane )
{
    HitInfo hit = { FLT_MAX, float3( 0, 0, 0 )};

    // d = b * n
    float d = dot( rayDirection, normalPlane );

    // n = (C - A) * n
    float n = dot( ( pointOnPlane - rayOrigin ), normalPlane );

    // Избегаем деления на ноль (параллельный луч)
    // Используем abs(d) > 0.0001 для устойчивости
    if ( abs( d ) > 0.0001 )
    {
        float t = n / d;

        // Пересечение должно быть перед камерой
        if ( t > 0.001 && t < FLT_MAX ) 
        {
            hit.t = t;
            hit.normal = normalPlane;
        }
    }
    return hit;
}

HitInfo IntersectPlane2( Ray ray, float3 normalPlane, float d)
{
    HitInfo hit = { FLT_MAX, float3( 0, 0, 0 ) };
    float dist = dot( normalPlane, ray.origin) - d;
    float dotND = dot( ray.direction, normalPlane );
        
    if ( abs( dotND ) > 0.0001 )
    {
        float t = dist / -dotND;

        // Пересечение должно быть перед камерой
        if ( t > 0.001 && t < FLT_MAX )
        {
            hit.t = t;
            hit.normal = normalPlane;
        }
    }
    return hit;
}

HitInfo IntefsectTriangle(Ray ray, Triangle tr)
{
    const HitInfo hitMax = { FLT_MAX, float3(0, 0, 0) };
    HitInfo hit = { FLT_MAX, float3(0, 0, 0)};
    const float3 normal = normalize(cross(tr.b - tr.a, tr.c - tr.a));
    const float d = dot(normal, tr.a);
    
    hit = IntersectPlane2(ray, normal, d);
    
    if (hit.t == FLT_MAX)
        return hitMax;
    float3 p = ray.origin + ray.direction* hit.t;
    if (dot(cross(tr.b - tr.a, p - tr.a), normal) < 0.0f)
        return hitMax;
    if (dot(cross(tr.c - tr.b, p - tr.b), normal) < 0.0f)
        return hitMax;
    if (dot(cross(tr.a - tr.c, p - tr.c), normal) < 0.0f)
        return hitMax;
    
    return hit;
}

float3 randomCosineWeightedVectorHemispher(float2 screenPos, float3 N)
{
    float r1 = RandomFloat(screenPos, frameTime);
    float r2 = RandomFloat(screenPos + 0.1, frameTime);
    
    // Преобразование в локальное пространство (y = cos(theta))
    float phi = 2.0 * PI * r1;
    float cosTheta = sqrt(r2); // Корень квадратный дает распределение cos-weighted
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    float x = cos(phi) * sinTheta;
    float z = sin(phi) * sinTheta;
    float y = cosTheta;
    
    // Создаем локальный базис (Tangent, Bitangent, Normal)
    float3 T, B;
    float sign = N.z >= 0.0 ? 1.0 : -1.0;
    const float a = -1.0 / (sign + N.z);
    const float b = N.x * N.y * a;
    T = float3(1.0 + sign * N.x * N.x * a, sign * b, -sign * N.x);
    B = float3(b, sign + sign * N.y * N.y * a, -sign * N.y);
    
    // Преобразование вектора из локального пространства в мировое
    return T * x + N * y + B * z;
}

float3 TraceScene( Ray ray, uint primitiveCount, uint trianglesCount, float2 screenPos, float depth )
{

    float3 throughput = float3(1, 1, 1);
    float3 radiance = float3(0, 0, 0);
    
    for (uint d = 0; d < depth; ++d)
    {
        int matIndex = -1;
        HitInfo closestHit = { FLT_MAX, float3(0, 0, 0) };
        for (uint i = 0; i < primitiveCount; i++)
        {
            Primitive p = ScenePrimitives[i];
            HitInfo currentHit;
            float3 color = SceneMaterials[p.matIndex].albedo;
            if (p.type == TYPE_SPHERE)
            {
                currentHit = IntersectSphere(ray, p.position_point.xyz, p.radius);
            }
            else if (p.type == TYPE_PLANE)
            {
                currentHit = IntersectPlane2(ray, p.position_point.xyz, p.radius);
            }

            if (currentHit.t > 0.001 && currentHit.t < closestHit.t)
            {
                closestHit = currentHit;
                matIndex = p.matIndex;
            }
        }
        for (uint i = 0; i < trianglesCount; i++)
        {
            Triangle tr = SceneTriangles[i];
            HitInfo currentHit = IntefsectTriangle(ray, tr);
        
            if (currentHit.t > 0.001 && currentHit.t < closestHit.t)
            {
                closestHit = currentHit;
                matIndex = tr.matIndex;
            }
        }
    
        if (closestHit.t == FLT_MAX || matIndex == -1)
        {
            radiance += throughput * enviroment;
            break;
        }
    
        if (dot(closestHit.normal, ray.direction) > 0.0)
            closestHit.normal = -closestHit.normal;
    
        float3 newDir = randomCosineWeightedVectorHemispher(screenPos, closestHit.normal);
        const float cosTheta = dot(newDir, closestHit.normal);


        const Material m = SceneMaterials[matIndex];
        // BRDF
        const float brdf = 1.0 / PI;
        const float pdf = cosTheta / PI;

        // Add emission
        radiance += throughput * m.emmision;
        // Update throughput
        throughput *= m.albedo;

        const float3 newOrig = ray.origin + ray.direction * closestHit.t + newDir * 1e-4;

        ray.origin = newOrig;
        ray.direction = newDir;
    }
    
    return radiance;
}

float3 getUniformSampleOffset( int index, int side_count )
{
    const float dist = 1.0 / float( side_count );
    const float halfDist = 0.5 * dist;

    const float x = float( index % side_count );
    const float y = float( index / side_count );

    return float3( ( halfDist + x * dist ) - 0.5, ( halfDist + y * dist ) - 0.5, 0.0 );
}
//---------------------------------------------------------------------------------
// Главная функция вычислительного шейдера
//--------------------------------------------------------------------------------------

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    OutputTexture.GetDimensions(width, height);


    if (dispatchThreadID.x >= width || dispatchThreadID.y >= height)
        return;

    float3 accumulatedColor = float3( 0, 0, 0 );
    for ( int s = 0; s < sampleCount * sampleCount; s++ )
    {
        float offset_u = getUniformSampleOffset(s, sampleCount ).x;
        float offset_v = getUniformSampleOffset( s, sampleCount ).y;

        float3 pixel_center = pixel00_loc
            + ( dispatchThreadID.x + offset_u ) * pixel_delta_u
            + ( dispatchThreadID.y + offset_v ) * pixel_delta_v;
        //Vector3 pixPos = pixel00_loc + Vector3( pixSize / 2.0f + pixel_delta_u * aspectRatio, -pixSize / 2.0f - v, 0.0f );
                
        Ray ray;
        ray.origin = camera_center;
        ray.direction = normalize(pixel_center - camera_center);
        
        accumulatedColor += TraceScene(ray, primitiveCount, trianglesCount, pixel_center.xy, 4);

    }

    OutputTexture[dispatchThreadID.xy] = float4(accumulatedColor / float( sampleCount * sampleCount ), 0.0);
}