---
tags:
  - camera
  - animation
  - mechanics
---
# Описание
___
**Camera Lag** — это механика сглаживания движения точки привязки Spring Arm. Камера следует за целью не жёстко: виртуальная точка привязки постепенно приближается к текущей позиции персонажа, создавая контролируемое ощущение массы и инерции.

Параметры для механики описаны [[Camera Lag (params)|тут]].

## Принцип работы
___

Текущая модель повторяет общий принцип Camera Lag в Unreal Spring Arm:

- сглаживается мировая позиция точки привязки, а не скорость персонажа и не вычисленный из скорости офсет;
- горизонтальная и вертикальная ошибки обрабатываются экспоненциальным сглаживанием со своими временами реакции;
- максимальное расстояние между реальной и сглаженной точками ограничивается;
- после вычисления сглаженной точки от неё откладывается обычная длина Spring Arm;
- коллизия камеры выполняется отдельным этапом и не изменяет внутреннее состояние Camera Lag.

Position Lag не различает разгон, остановку и смену направления. Для разгона, остановки и смены направления используются одни и те же настройки горизонтального и вертикального отклика. Это исключает ошибочную классификацию поворотов как Build или Recovery и не сохраняет отдельный устаревший вектор направления движения.

Горизонтальное следование одинаково по мировым осям X и Y. Вертикальное настраивается отдельно по Z. **Max Pivot Lag (m)** ограничивает общее расстояние между реальной и сглаженной точками.

Rotation Lag является отдельной частью механики. Он сглаживает запрошенные игроком углы Yaw и Pitch, не изменяя Position Lag. Активный [[Автоповорот Камеры]] сглаживает Yaw самостоятельно, а [[Восстановление Камеры]] — Pitch. Повторное сглаживание этих осей не применяется.

>[!info] Время реакции
>Время описывает устранение примерно 95% фиксированной ошибки. Для поворота отсчёт рассматривается из состояния покоя. При движущейся цели и работающих ограничениях это не гарантированное время достижения цели.

Так выглядит Camera Lag в игре Elden Ring: Nightreign:
<iframe id="ytplayer" type="text/html" width="720" height="405"
src="https://www.youtube.com/embed/?autoplay=1&enablejsapi=1&loop=1&playlist=jyrCtQ3-mro&controls=0"
frameborder="0" allowfullscreen></iframe>
А так выглядит Camera Lag в игре если включен [[Автоповорот Камеры]]:
<iframe id="ytplayer" type="text/html" width="720" height="405"
src="https://www.youtube.com/embed/?autoplay=1&enablejsapi=1&loop=1&playlist=C2DjBjtWNcQ&controls=0"
frameborder="0" allowfullscreen></iframe>
А вот демонстрация Camera Rotation Lag в Unreal:
<iframe id="ytplayer" type="text/html" width="720" height="405"
src="https://www.youtube.com/embed/?autoplay=1&enablejsapi=1&loop=1&playlist=ahPor5qXibE&controls=0"
frameborder="0" allowfullscreen></iframe>
# См. Также
___
Пример реализации механики в Unreal Engine:
- [Spring Arm](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-spring-arm-components?application_version=4.27)
- [Spring Arm API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/USpringArmComponent?application_version=4.27)
- [Spring Arm Component](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/Engine/Private/GameFramework/SpringArmComponent.cpp)

>[!info] Есть Нюанс!
> В Unreal нет отдельной реализации Camera Lag но эта механика встроена в Spring Arm Component.

Параметры:
- [[Camera Lag (params)]]
