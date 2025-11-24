// Максимальное значение для float
#define FLT_MAX 3.402823466e+38F
#define TYPE_SPHERE 0
#define TYPE_PLANE 1

// Выходная текстура (RW - Read/Write)
RWTexture2D<float4> OutputTexture : register(u0);

// Структура для результата пересечения
struct HitInfo
{
    float t; // Расстояние до точки пересечения
    float3 normal; // Нормаль в точке пересечения
    float3 color; // Цвет поверхности
};


// Структура для Буфера Констант (должна совпадать с C++ структурой)
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
};

struct Primitive
{
    uint type;
    float radius;
    float _pad0[2];
    float4 position_point;
    float4 normal_color;
};

struct Triangle
{
    float3 a;
    float _padA;
    float3 b;
    float _padB;
    float3 c;
    float _padC;
    float3 color;
    float _padColort;
};

struct Ray
{
    float3 origin;
    float3 direction;
};

// Structured Buffer (Регистр t0: Shader Resource View)
StructuredBuffer<Primitive> ScenePrimitives : register( t0 );
StructuredBuffer<Triangle> SceneTriangles : register( t1 );
//--------------------------------------------------------------------------------------
// Вспомогательные Функции
//--------------------------------------------------------------------------------------

// Функция для трассировки луча со сферой (Оптимизирована: a = 1.0)
HitInfo IntersectSphere(Ray ray, float3 sphereCenter, float sphereRadius)
{
    HitInfo hit = { FLT_MAX, float3(0, 0, 0), float3(0, 0, 0) };

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
    HitInfo hit = { FLT_MAX, float3( 0, 0, 0 ), float3( 0, 0, 0 ) };

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
            hit.normal = normalPlane; // Сохраняем нормаль для последующего освещения
            hit.color = float3( 1.0, 1.0, 0.0 );
        }
    }
    return hit;
}

HitInfo IntersectPlane2( Ray ray, float3 normalPlane, float d, float3 color )
{
    HitInfo hit = { FLT_MAX, float3( 0, 0, 0 ), float3( 0, 0, 0 ) };
    float dist = dot( normalPlane, ray.origin) - d;
    float dotND = dot( ray.direction, normalPlane );
        
    if ( abs( dotND ) > 0.0001 )
    {
        float t = dist / -dotND;

        // Пересечение должно быть перед камерой
        if ( t > 0.001 && t < FLT_MAX )
        {
            hit.t = t;
            hit.normal = normalPlane; // Сохраняем нормаль для последующего освещения
            hit.color = color;
        }
    }
    return hit;
}

HitInfo IntefsectTriangle(Ray ray, Triangle tr)
{
    const HitInfo hitMax = { FLT_MAX, float3(0, 0, 0), float3(0, 0, 0) };
    HitInfo hit = { FLT_MAX, float3(0, 0, 0), float3(0, 0, 0) };
    const float3 normal = normalize(cross(tr.b - tr.a, tr.c - tr.a));
    const float d = dot(normal, tr.a);
    
    hit = IntersectPlane2(ray, normal, d, tr.color);
    
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

HitInfo TraceScene( Ray ray, uint primitiveCount, uint trianglesCount )
{
    HitInfo closestHit = { FLT_MAX, float3( 0,0,0 ), float3( 0,0,0 ) };

    for ( uint i = 0; i < primitiveCount; i++ )
    {
        Primitive p = ScenePrimitives[i];
        HitInfo currentHit;

        if ( p.type == TYPE_SPHERE )
        {
            // Используем xyz компоненты
            currentHit = IntersectSphere( ray, p.position_point.xyz, p.radius );
            currentHit.color = p.normal_color;
            closestHit.normal = currentHit.normal;
        }
        else if ( p.type == TYPE_PLANE )
        {
            currentHit = IntersectPlane2( ray, p.position_point.xyz, p.radius, p.normal_color.xyz );
            closestHit.normal = p.normal_color.xyz;
            float3 pos = ray.origin + ray.direction * currentHit.t;
            if ( ( int( pos.x + 1000 ) % 2 ) != ( int( pos.z + 1000 ) % 2 ) )
            {
                currentHit.color = float3( 0, 0, 0 );
            }
        }

        if ( currentHit.t > 0.001 && currentHit.t < closestHit.t )
        {
            closestHit = currentHit;
        }
    }
    for (uint i = 0; i < trianglesCount; i++)
    {
        Triangle tr = SceneTriangles[i];
        HitInfo currentHit = IntefsectTriangle(ray, tr);
        
        if (currentHit.t > 0.001 && currentHit.t < closestHit.t)
        {
            closestHit = currentHit;
        }
    }
    return closestHit;
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

        // --- 1. Генерация Луча (Vector Ray Generation) ---
        // 1. Определяем центр пикселя в 3D пространстве
        float3 pixel_center = pixel00_loc
            + ( dispatchThreadID.x + offset_u ) * pixel_delta_u
            + ( dispatchThreadID.y + offset_v ) * pixel_delta_v;
        //Vector3 pixPos = pixel00_loc + Vector3( pixSize / 2.0f + pixel_delta_u * aspectRatio, -pixSize / 2.0f - v, 0.0f );
                
        Ray ray;
        ray.origin = camera_center;
        ray.direction = normalize(pixel_center - camera_center);
        HitInfo hit = TraceScene( ray, primitiveCount, trianglesCount );

        float4 color;

        float a = 0.5 * ( ray.direction.y + 1.0 );
        float3 background = ( 1.0 - a ) * float3( 1.0, 1.0, 1.0 ) + a * float3( 0.5, 0.7, 1.0 ); // От белого до светло-голубого

        color = float4( background, 1.0 );
        // --- 3. Вычисление Цвета и Визуализация Нормалей ---
        if ( hit.t > 0.0 && hit.t < FLT_MAX )
        {
            color = float4( hit.color, 1.0 );
        }
        accumulatedColor += color;
    }

    OutputTexture[dispatchThreadID.xy] = float4(accumulatedColor / float( sampleCount * sampleCount ), 0.0);
}