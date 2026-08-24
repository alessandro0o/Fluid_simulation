#pragma once


Color getColor(float value)
{
    const int gray = 128;

    float v = value / 5.0f;
    if (v < -1.0f) v = -1.0f;
    if (v > 1.0f) v = 1.0f;

    Color c;

    if (v < 0.0f)
    {
        float t = -v;
        c.r = gray + static_cast<int>((255 - gray) * t);
        c.g = gray - static_cast<int>(gray * t);
        c.b = gray - static_cast<int>(gray * t);
    }
    else
    {
        float t = v;
        c.r = gray - static_cast<int>(gray * t);
        c.g = gray - static_cast<int>(gray * t);
        c.b = gray + static_cast<int>((255 - gray) * t);
    }
    c.a = 255;

    return c;
}