#ifndef ESP_H
#define ESP_H

struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };

Vector3 enemyPos = GetEnemyPosition(i); // Read raw memory bytes
D3DMATRIX viewMatrix = GetViewMatrix(); // Read from LocalPlayer / CameraManager

Vector2 screenPos;
if (WorldToScreen(enemyPos, screenPos, viewMatrix)) {
    // If the coordinates fit your resolution boundaries, they are "in view"
    DrawBox(screenPos.x, screenPos.y, 50.0f, 100.0f, Color::Red);
}


#endif // ESP_H
