---
aliases:
tags:
  - camera
  - params
---
### 1. Enable Min angle Bias
___
**Type**: `bool`
**Default**: `true`

**Описание**:
Включает Min Angle Bias. При выключении поле диапазона скрывается, сохраняя значение.
### 2. Min Angle Bias Offset
___
**Type:** `Float`  
**Default:** `15`

**Описание:**  
Угол, определяющий размер диапазона от минимального вертикального угла (Pitch), в пределах которого применяется Min Angle Bias. Измеряется в градусах. Значение 0 отключает смещение.
>[!info] В разработке
>Следующие параметры относятся к проектируемому Max Angle Bias. В текущем компоненте их нет.

### 3. Enable Max angle Bias
___
**Type**: `bool`
**Default**: `false`

**Описание**:
Ключ, отвечающий за включение и выключение механики Max Angle Bias.  
### 4. Max angle Bias Offset
___
**Type:** `Float`  
**Default:** `0`

**Описание:**  
Максимальное расстояние (в юнитах), на которое камера может смещаться вперёд вдоль forward-вектора персонажа при активном Max Angle Bias.

### 5. MaxBias Depth Limit
___
**Type:** `Float`
**Default:** `5` (Units)

**Описание:**
Минимальный перепад высоты в юнитах на расстоянии **Max angle Bias Offset**, необходимый для активации **Max Angle Bias**.
# См. Также
---

- [[Vertical Camera Bias]]
