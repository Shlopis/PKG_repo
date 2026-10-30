#include "rasterengine.h"
#include <cmath>
#include <algorithm>
#include <string>

namespace RasterEngine {

// ПОШАГОВЫЙ АЛГОРИТМ
void lineStep(int x0, int y0, int x1, int y1, IRasterTarget& target) {
    int dx = x1 - x0, dy = y1 - y0;
    if (dx == 0 && dy == 0){
        target.SetPixel(x0, y0, 1.0);
        return;
    }

    if (abs(dx) >= abs(dy)) {
        // шаг по X
        if (x0 > x1){
            std::swap(x0, x1);
            std::swap(y0, y1);
            dx = -dx;
            dy = -dy;
        }
        double k = (dx == 0) ? 0.0 : double(dy) / dx;
        for (int x = x0; x <= x1; ++x) {
            double y = y0 + k * (x - x0);
            target.SetPixel(x, int(round(y)), 1.0);
        }
    }
    else {
        // шаг по Y
        if (y0 > y1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
            dx = -dx; dy = -dy;
        }
        double k = (dy == 0) ? 0.0 : double(dx) / dy;
        for (int y = y0; y <= y1; ++y) {
            double x = x0 + k * (y - y0);
            target.SetPixel(int(round(x)), y, 1.0);
        }
    }
}

//ЦДА   (x_i , y_i) = (round(x_a + dx/L ), round (y_a + dy/L))
void lineDDA(int x0, int y0, int x1, int y1, IRasterTarget& target) {
    int dx = x1 - x0, dy = y1 - y0;
    int steps = std::max(abs(dx), abs(dy));
    if (steps == 0) {
        target.SetPixel(x0, y0, 1.0);
        return;
    }

    double xInc = double(dx) / steps;
    double yInc = double(dy) / steps;
    double x = x0, y = y0;

    for (int i = 0; i <= steps; ++i) {
        target.SetPixel(int(round(x)), int(round(y)), 1.0);
        x += xInc;
        y += yInc;
    }
}

//БРЕЗЕНХЕМ
void lineBresenham(int x0, int y0, int x1, int y1, IRasterTarget& target) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        target.SetPixel(x0, y0, 1.0);
        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 >= -dy) { err -= dy; x0 += sx; }   // шаг по Y
        if (e2 <=  dx) { err += dx; y0 += sy; }   // шаг по X
    }
}

//8-октантная симметрия
static void plotOctants(int x, int y, IRasterTarget& t) {
    t.SetPixel( x,  y, 1.0);
    t.SetPixel(-x,  y, 1.0);
    t.SetPixel( x, -y, 1.0);
    t.SetPixel(-x, -y, 1.0);
    t.SetPixel( y,  x, 1.0);
    t.SetPixel(-y,  x, 1.0);
    t.SetPixel( y, -x, 1.0);
    t.SetPixel(-y, -x, 1.0);
}

//ОКРУЖНОСТЬ БРЕЗЕНХЕМА
void circleBresenham(int r, IRasterTarget& target) {
    if (r < 0) return;
    if (r == 0){
        target.SetPixel(0, 0, 1.0);
        return;
    }
    int x = 0, y = r;
    int p = 1 - r;
    while (x <= y) {
        plotOctants(x, y, target);
        ++x;
        if (p < 0) {
            p += 2 * x + 1; // E
        }
        else {
            --y;
            p += 2 * (x - y) + 1; //SE
        }
    }
}

static std::string reversed(const std::string& s) {
    return std::string(s.rbegin(), s.rend());
}

//АЛГОРИТМ КАСТЛА–ПИТВЕЯ
void lineCastlePitwey(int x0, int y0, int x1, int y1, IRasterTarget& target) {
    int dx = x1 - x0, dy = y1 - y0;
    if (dx == 0 && dy == 0) { target.SetPixel(x0, y0, 1.0); return; }

    const int sx = (dx > 0) ? 1 : -1;
    const int sy = (dy > 0) ? 1 : -1;
    const int a = abs(dx), b = abs(dy);
    const bool ocb = (a >= b);  // ось A кода: X (иначе Y)

    const int A = std::max(a, b), B = std::min(a, b);
    std::string code;
    if (B == 0)
        code = std::string(A, 'S'); // вырожденный случай
    else if (A == B)
        code = std::string(A, 'D'); // диагональ
    else {
        std::string m1 = "S";
        std::string m2 = "D";
        int x = A - B;
        int y = B;
        while (true) {
            if (x > y) {
                m2 = m1 + reversed(m2);
                x -= y;
            } else if (x < y) {
                m1 = m2 + reversed(m1);
                y -= x;
            } else {
                std::string block = m2 + reversed(m1);
                for (int i = 0; i < x; ++i)
                    code += block;
                break;
            }
        }
    }
    //раскодирование: S/D
    int cx = 0, cy = 0;
    target.SetPixel(x0, y0, 1.0);
    for (char c : code) {
        if (ocb) {   // ось A = X: S - вправо, D - диагональ
            ++cx;
            if (c == 'D')
                ++cy;
        }
        else {  // ось A = Y: S - вверх, D - диагональ
            ++cy;
            if (c == 'D')
                ++cx;
        }
        target.SetPixel(x0 + sx * cx, y0 + sy * cy, 1.0);
    }
}

//ЛИНИЯ ВУ
void lineWu(int x0, int y0, int x1, int y1, IRasterTarget& target) {
    int dx = x1 - x0, dy = y1 - y0;
    // Вырожденные случаи
    if (dx == 0) {
        if (dy < 0) std::swap(y0, y1);
        for (int y = y0; y <= y1; ++y)
            target.SetPixel(x0, y, 1.0);
        return;
    }
    if (dy == 0) {
        if (dx < 0) std::swap(x0, x1);
        for (int x = x0; x <= x1; ++x)
            target.SetPixel(x, y0, 1.0);
        return;
    }

    if (std::abs(dx) >= std::abs(dy)) {
        // шаг по X
        if (x0 > x1){
            std::swap(x0, x1);
            std::swap(y0, y1);
            dx = -dx; dy = -dy;
        }
        double gradient = double(dy) / double(dx);
        double yIdeal = y0;
        for (int x = x0; x <= x1; ++x) {
            int yi = int(std::floor(yIdeal));
            double frac = yIdeal - yi;
            target.SetPixel(x, yi, 1.0 - frac);  // основной
            target.SetPixel(x, yi + 1, frac);   // граничный
            yIdeal += gradient;
        }
    }
    else {
        // шаг по Y
        if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); dx = -dx; dy = -dy; }
        double gradient = double(dx) / double(dy);
        double xIdeal = x0;
        for (int y = y0; y <= y1; ++y) {
            int xi = int(std::floor(xIdeal));
            double frac = xIdeal - xi;
            target.SetPixel(xi, y, 1.0 - frac);
            target.SetPixel(xi + 1, y, frac);
            xIdeal += gradient;
        }
    }

    // Точные концы отрезка
    target.SetPixel(x0, y0, 1.0);
    target.SetPixel(x1, y1, 1.0);
}

//8 октантов с интенсивностью
static void plotOctantsI(int x, int y, double i, IRasterTarget& t) {
    t.SetPixel( x,  y, i);
    t.SetPixel(-x,  y, i);
    t.SetPixel( x, -y, i);
    t.SetPixel(-x, -y, i);
    t.SetPixel( y,  x, i);
    t.SetPixel(-y,  x, i);
    t.SetPixel( y, -x, i);
    t.SetPixel(-y, -x, i);
}

//ОКРУЖНОСТЬ ВУ
void circleWu(int r, IRasterTarget& target) {
    if (r < 0) return;
    if (r == 0) { target.SetPixel(0, 0, 1.0); return; }
    int x = 0;
    double yIdeal = double(r);
    while (x <= int(yIdeal)) {
        int y0 = int(std::floor(yIdeal));
        double frac = yIdeal - y0;
        plotOctantsI(x, y0, 1.0 - frac, target);
        plotOctantsI(x, y0 + 1, frac, target);
        ++x;
        yIdeal = std::sqrt(double(r * r - x * x));
    }
}

} // namespace RasterEngine
