---
tags:
  - camera
  - mechanics
  - shading
  - character
---
# Описание
___
Механика обеспечивает **плавное уменьшение прозрачности персонажа**, когда камера приближается слишком близко.  
Цель — сохранить читаемость сцены и избежать перекрытия камеры и модели персонажа, не мешая игровому процессу.

Параметры описаны [[Hiding Character (params)|тут]].

Демо:
<iframe width="560" height="315" src="https://www.youtube.com/embed/_hbVDKHGG8s?si=U1J0W0V-D_w9FpBM&amp;clip=Ugkxnoj5kYbNOZ9ObrMDCkkNQjJtg8VKhVTy&amp;controls=0&amp;autoplay=1&amp;mute=1&amp;clipt=EK7zCBjb1gk" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay=1; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen loop = 1></iframe>
## Принцип работы
---

- Система отслеживает расстояние между камерой и персонажем.

- Если дистанция становится меньше заданного порога, прозрачность персонажа начинает плавно уменьшаться до минимального значения.

- При увеличении расстояния прозрачность постепенно возвращается к нормальной.

- Эффект действует только на визуальное представление персонажа и не влияет на его коллайдер или взаимодействие с миром.

## Использование
---

- Особенно полезно при движении в дверных проёмах или узких коридорах.

- Может работать вместе с Camera Lag, Smooth Return After Collision и World Space Masking без конфликтов.

- Применяется как для основной модели персонажа, так и для аксессуаров или экипировки, чтобы сохранять целостность визуала.

# См. Также
---
- [[Hiding Character (params)]]