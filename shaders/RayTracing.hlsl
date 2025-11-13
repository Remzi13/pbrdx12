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
    float3 _pad5;
};

struct Primitive
{
    uint type;
    float radius;
    float _pad0[2];
    float4 position_point;
    float4 normal_color;
};

// Structured Buffer (Регистр t0: Shader Resource View)
StructuredBuffer<Primitive> SceneData : register( t0 );
//--------------------------------------------------------------------------------------
// Вспомогательные Функции
//--------------------------------------------------------------------------------------

// Функция для трассировки луча со сферой (Оптимизирована: a = 1.0)
HitInfo IntersectSphere(float3 rayOrigin, float3 rayDirection, float3 sphereCenter, float sphereRadius)
{
    HitInfo hit = { FLT_MAX, float3(0, 0, 0), float3(0, 0, 0) };

    // Вектор от начала луча до центра сферы: oc = A - C
    float3 oc = rayOrigin - sphereCenter;

    // Квадратное уравнение: at^2 + bt + c = 0
    // Так как rayDirection нормализован, a = 1.0. 
    float b = 2.0 * dot(oc, rayDirection);
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
            
            float3 hitPoint = rayOrigin + rayDirection * t;
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

HitInfo TraceScene( float3 rayOrigin, float3 rayDirection, uint primitiveCount )
{
    HitInfo closestHit = { FLT_MAX, float3( 0,0,0 ), float3( 0,0,0 ) };

    // primitiveCount берется из cbuffer SceneConstants
    for ( uint i = 0; i < primitiveCount; i++ )
    {
        Primitive p = SceneData[i];
        HitInfo currentHit;

        if ( p.type == TYPE_SPHERE )
        {
            // Используем xyz компоненты
            currentHit = IntersectSphere( rayOrigin, rayDirection, p.position_point.xyz, p.radius );
            closestHit.normal = currentHit.normal;
        }
        else if ( p.type == TYPE_PLANE )
        {
            currentHit = IntersectPlane( rayOrigin, rayDirection, p.position_point.xyz, p.normal_color.xyz );
            closestHit.normal = p.normal_color.xyz;
        }

        if ( currentHit.t > 0.001 && currentHit.t < closestHit.t )
        {
            closestHit = currentHit;
            // Устанавливаем цвет из структуры для визуализации
            closestHit.color = p.normal_color.xyz;
        }
    }
    return closestHit;
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

    // --- 1. Генерация Луча (Vector Ray Generation) ---
    // 1. Определяем центр пикселя в 3D пространстве
    float3 pixel_center = pixel00_loc
                        + dispatchThreadID.x * pixel_delta_u
                        + dispatchThreadID.y * pixel_delta_v;

    // 2. Начало луча (Origin)

    float3 rayOrigin = camera_center;

    // 3. Направление луча (Direction) - Должно быть нормализовано!
    float3 rayDirection = normalize(pixel_center - rayOrigin);
        
    
    //HitInfo hit = IntersectSphere(rayOrigin, rayDirection, sphereCenter, sphereRadius);
    //HitInfo hit = IntersectPlane( rayOrigin, rayDirection, float3( 0.0f, -1.0f, 0.0f ), float3( 0.0f, 1.0f, 0.0f ) );
    HitInfo hit = TraceScene( rayOrigin, rayDirection, primitiveCount);

    float4 finalColor;

    float a = 0.5 * ( rayDirection.y + 1.0 );
    float3 background = ( 1.0 - a ) * float3( 1.0, 1.0, 1.0 ) + a * float3( 0.5, 0.7, 1.0 ); // От белого до светло-голубого

    finalColor = float4( background, 1.0 );
    // --- 3. Вычисление Цвета и Визуализация Нормалей ---
    if ( hit.t > 0.0 && hit.t < FLT_MAX )
    {     
        finalColor = float4(hit.color, 1.0);
    }

    OutputTexture[dispatchThreadID.xy] = finalColor;
}