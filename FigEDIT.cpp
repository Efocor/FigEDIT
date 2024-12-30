/*
Código hecho por: FECORO/Felipe Alexander Correa Rodríguez o FERI
- Last update: 2024

El código es una prueba de concepto para un editor de formas en 2D con SFML e ImGui.
Se pueden añadir círculos, rectángulos y triángulos, moverlos, rotarlos y escalarlos.
También se pueden guardar y cargar las formas a/desde un archivo de texto.
El código es un ejemplo básico y no incluye todas las características de un editor completo.

Tiene funciones aún no bien implementadas como lo son el cambio de forma, el deshacer y rehacer, etc.
Se pueden hacer animaciones a las formas, pero solo de tipo rotativa o de escala pulsante.

El código ocupa SFML en su versión 2.6.2 y ImGui en su versión 1.8x, compatible con SFML.

Este código fue una prueba de computación gráfica y no se recomienda su uso en producción.
Se trata de un código de prueba y no se garantiza su correcto funcionamiento en todos los casos.
Adicionalmente, vale destacar que el código ocupa figuras geométricas básicas y no figuras complejas.
Pero fácilmente podría añadirse opciones para un polígono maestro y cambiar su número de lados.
Lo anterior haría que el código sea más complejo, al tener una mayor gama de figuras.

Si se implementa o une con GLEW y GLM, se podrían hacer figuras en 3D y con texturas.
Aunque actualmente no se ha implementado, se podría hacer una figura en Pseudo 3D con SFML solamente.

*/


//...........................editor.cpp............................................

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <algorithm>
#include <cmath>
#include <stack>
#include <utility>

//....................enumeración para los tipos de formas disponibles
enum class ShapeType {
    Circle,
    Rectangle,
    Triangle,
    Ellipse,
    Polygon,
    Line,
    Cube,
    Text
};

//....................clase base para formas
class ShapeBase {
public:
    ShapeBase(ShapeType type, sf::Vector2f position, sf::Color color)
        : type(type), position(position), color(color), rotation(0.0f), scale(1.0f, 1.0f), isSelected(false), isAnimated(false) {}
    virtual ~ShapeBase() {}
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual void updateShape(float deltaTime) = 0;

    void setPosition(sf::Vector2f pos) {
        position = pos;
        shapePtr->setPosition(position);
    }

    void setRotation(float rot) {
        rotation = rot;
        shapePtr->setRotation(rotation);
    }

    void setScale(sf::Vector2f scl) {
        scale = scl;
        shapePtr->setScale(scale);
    }

    void setColor(sf::Color col) {
        color = col;
        shapePtr->setFillColor(color);
    }

    sf::Vector2f getPosition() const { return position; }
    float getRotation() const { return rotation; }
    sf::Vector2f getScale() const { return scale; }
    sf::Color getColor() const { return color; }
    ShapeType getType() const { return type; }

    sf::Shape* getShapePtr() const { return shapePtr.get(); }

    void select() { isSelected = true; updateSelectionVisual(); }
    void deselect() { isSelected = false; updateSelectionVisual(); }
    bool selected() const { return isSelected; }

    //...método para clonar formas
    virtual std::unique_ptr<ShapeBase> clone() const = 0;

    //...animación
    void enableAnimation(bool enable) { isAnimated = enable; }
    bool animated() const { return isAnimated; }

protected:
    ShapeType type;
    std::unique_ptr<sf::Shape> shapePtr;
    sf::Vector2f position;
    sf::Color color;
    float rotation;
    sf::Vector2f scale;
    bool isSelected;
    bool isAnimated;

    //método para actualizar la apariencia cuando es seleccionado
    void updateSelectionVisual() {
        if (isSelected) {
            shapePtr->setOutlineThickness(3.0f);
            shapePtr->setOutlineColor(sf::Color::Yellow);
        }
        else {
            shapePtr->setOutlineThickness(0.0f);
            shapePtr->setOutlineColor(sf::Color::Transparent);
        }
    }
};

//..........Clase para Círculos
class CircleShapeClass : public ShapeBase {
public:
    CircleShapeClass(sf::Vector2f position, sf::Color color, float radius = 50.0f)
        : ShapeBase(ShapeType::Circle, position, color), radius(radius), rotationSpeed(0.0f), scaleSpeed(0.0f) {
        shapePtr = std::make_unique<sf::CircleShape>(radius);
        shapePtr->setFillColor(color);
        shapePtr->setOrigin(radius, radius);
        shapePtr->setPosition(position);
    }

    void draw(sf::RenderWindow& window) override {
        shapePtr->setRotation(rotation);
        shapePtr->setScale(scale);
        shapePtr->setFillColor(color);
        window.draw(*shapePtr);
    }

    void updateShape(float deltaTime) override {
        if (isAnimated) {
            //..........Rotación continua
            rotation += rotationSpeed * deltaTime;
            if (rotation > 360.0f) rotation -= 360.0f;
            shapePtr->setRotation(rotation);

            //..........Escala pulsante
            float scaleFactor = 1.0f + std::sin(scaleTime + deltaTime) * scaleSpeed * deltaTime;
            shapePtr->setScale(scaleFactor * scale.x, scaleFactor * scale.y);
            scaleTime += deltaTime;
        }
    }

    void setRadius(float r) {
        radius = r;
        static_cast<sf::CircleShape*>(shapePtr.get())->setRadius(radius);
        shapePtr->setOrigin(radius, radius);
    }

    //clonacion
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<CircleShapeClass>(position, color, radius);
    }

    float getRadius() const { return radius; }

    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    float getRotationSpeed() const { return rotationSpeed; }

    void setScaleSpeed(float speed) { scaleSpeed = speed; }
    float getScaleSpeed() const { return scaleSpeed; }

private:
    float radius;
    float rotationSpeed; //..........Velocidad de rotación (grados por segundo)
    float scaleSpeed;    //..........Velocidad de cambio de escala
    float scaleTime = 0.0f;
};

//..........Clase para Cubo en Pseudo 3D
class CubeShapeClass : public ShapeBase {
public:
    CubeShapeClass(sf::Vector2f position, sf::Color color, float size = 100.0f, float depth = 50.0f)
        : ShapeBase(ShapeType::Polygon, position, color), size(size), depth(depth), rotationAngle(0.0f) {
        initializeCube();
    }

    //..........Constructor de copia personalizado
    CubeShapeClass(const CubeShapeClass& other)
        : ShapeBase(other.type, other.position, other.color), size(other.size), depth(other.depth), rotationAngle(other.rotationAngle) {
        initializeCube();
    }

    //..........Implementación del método de clonación
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<CubeShapeClass>(*this);
    }

    void draw(sf::RenderWindow& window) override {
        for (const auto& line : lines) {
            window.draw(line);
        }
    }

    void updateShape(float deltaTime) override {
        //..........Implementar lógica de rotación continua si se desea
    }

    //..........Método para rotar el cubo
    void rotate(float angle) {
        rotationAngle += angle;
        if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        if (rotationAngle < 0.0f) rotationAngle += 360.0f;
        applyRotation();
    }

private:
    float size;          //..........Tamaño del cubo
    float depth;         //..........Profundidad para el efecto 3D
    float rotationAngle; //..........Ángulo de rotación en grados
    std::vector<sf::VertexArray> lines; //..........Líneas que forman el cubo

    //..........Inicializar las líneas del cubo
    void initializeCube() {
        //..........Definir los ocho vértices del cubo
        sf::Vector2f frontTopLeft = sf::Vector2f(-size / 2, -size / 2);
        sf::Vector2f frontTopRight = sf::Vector2f(size / 2, -size / 2);
        sf::Vector2f frontBottomLeft = sf::Vector2f(-size / 2, size / 2);
        sf::Vector2f frontBottomRight = sf::Vector2f(size / 2, size / 2);

        sf::Vector2f backTopLeft = frontTopLeft + sf::Vector2f(depth, depth);
        sf::Vector2f backTopRight = frontTopRight + sf::Vector2f(depth, depth);
        sf::Vector2f backBottomLeft = frontBottomLeft + sf::Vector2f(depth, depth);
        sf::Vector2f backBottomRight = frontBottomRight + sf::Vector2f(depth, depth);

        //..........Crear las líneas del cubo
        addLine(frontTopLeft, frontTopRight);
        addLine(frontTopRight, frontBottomRight);
        addLine(frontBottomRight, frontBottomLeft);
        addLine(frontBottomLeft, frontTopLeft);

        addLine(backTopLeft, backTopRight);
        addLine(backTopRight, backBottomRight);
        addLine(backBottomRight, backBottomLeft);
        addLine(backBottomLeft, backTopLeft);

        addLine(frontTopLeft, backTopLeft);
        addLine(frontTopRight, backTopRight);
        addLine(frontBottomLeft, backBottomLeft);
        addLine(frontBottomRight, backBottomRight);

        //..........Aplicar posición inicial
        applyRotation();
    }

    //..........Añadir una línea al cubo
    void addLine(const sf::Vector2f& start, const sf::Vector2f& end) {
        sf::VertexArray line(sf::Lines, 2);
        line[0].position = start;
        line[0].color = color;
        line[1].position = end;
        line[1].color = color;
        lines.push_back(line);
    }

    //..........Aplicar rotación a todas las líneas del cubo
    void applyRotation() {
        float rad = rotationAngle * 3.14159265f / 180.0f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);

        for (auto& line : lines) {
            for (int i = 0; i < 2; ++i) {
                sf::Vector2f originalPos = getOriginalPosition(line[i].position);
                sf::Vector2f rotatedPos;
                rotatedPos.x = originalPos.x * cosA - originalPos.y * sinA;
                rotatedPos.y = originalPos.x * sinA + originalPos.y * cosA;
                line[i].position = rotatedPos + position;
            }
        }
    }

    //..........Obtener la posición original sin rotación
    sf::Vector2f getOriginalPosition(const sf::Vector2f& rotatedPos) const {
        float rad = -rotationAngle * 3.14159265f / 180.0f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);
        sf::Vector2f originalPos;
        originalPos.x = (rotatedPos.x - position.x) * cosA - (rotatedPos.y - position.y) * sinA;
        originalPos.y = (rotatedPos.x - position.x) * sinA + (rotatedPos.y - position.y) * cosA;
        return originalPos;
    }
};


//..........Clase para Textos
class TextShapeClass : public ShapeBase {
public:
    TextShapeClass(sf::Vector2f position, sf::Color color, const std::string& content = "Texto", const sf::Font& font = sf::Font()) 
        : ShapeBase(ShapeType::Text, position, color), content(content), font(font), characterSize(24) {
        textPtr = std::make_unique<sf::Text>();
        textPtr->setFont(this->font);
        textPtr->setString(content);
        textPtr->setFillColor(color);
        textPtr->setCharacterSize(characterSize);
        sf::FloatRect bounds = textPtr->getLocalBounds();
        textPtr->setOrigin(bounds.width / 2, bounds.height / 2);
        textPtr->setPosition(position);
    }

    void draw(sf::RenderWindow& window) override {
        textPtr->setRotation(rotation);
        textPtr->setScale(scale);
        textPtr->setFillColor(color);
        window.draw(*textPtr);
    }

    void updateShape(float deltaTime) override {
        if (isAnimated) {
            //..........Ejemplo: Parpadeo de texto
            blinkTimer += deltaTime;
            if (blinkTimer >= blinkInterval) {
                visible = !visible;
                textPtr->setFillColor(visible ? color : sf::Color::Transparent);
                blinkTimer = 0.0f;
            }
        }
    }

    void setContent(const std::string& newContent) {
        content = newContent;
        textPtr->setString(content);
        sf::FloatRect bounds = textPtr->getLocalBounds();
        textPtr->setOrigin(bounds.width / 2, bounds.height / 2);
    }

    std::string getContent() const { return content; }

    void setCharacterSize(unsigned int size) {
        characterSize = size;
        textPtr->setCharacterSize(characterSize);
        sf::FloatRect bounds = textPtr->getLocalBounds();
        textPtr->setOrigin(bounds.width / 2, bounds.height / 2);
    }

    //clonacion sin override:
    std::unique_ptr<ShapeBase> clone() const override {
       return std::make_unique<TextShapeClass>(position, color, content, font);
    }

    unsigned int getCharacterSize() const { return characterSize; }

    void setBlinkInterval(float interval) { blinkInterval = interval; }

private:
    std::unique_ptr<sf::Text> textPtr;
    std::string content;
    sf::Font font;
    unsigned int characterSize;
    float blinkTimer = 0.0f;
    float blinkInterval = 0.5f; //..........Intervalo de parpadeo en segundos
    bool visible = true;
};

//..........Clase para Rectángulos
class RectangleShapeClass : public ShapeBase {
public:
    RectangleShapeClass(sf::Vector2f position, sf::Color color, sf::Vector2f size = sf::Vector2f(100.0f, 60.0f))
        : ShapeBase(ShapeType::Rectangle, position, color), size(size), rotationSpeed(0.0f), scaleSpeed(0.0f) {
        shapePtr = std::make_unique<sf::RectangleShape>(size);
        shapePtr->setFillColor(color);
        shapePtr->setOrigin(size.x / 2, size.y / 2);
        shapePtr->setPosition(position);
    }

    //clonacion sin override:
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<RectangleShapeClass>(position, color, size);
    }

    void draw(sf::RenderWindow& window) override {
        shapePtr->setRotation(rotation);
        shapePtr->setScale(scale);
        shapePtr->setFillColor(color);
        window.draw(*shapePtr);
    }

    void updateShape(float deltaTime) override {
        if (isAnimated) {
            //..........Rotación continua
            rotation += rotationSpeed * deltaTime;
            if (rotation > 360.0f) rotation -= 360.0f;
            shapePtr->setRotation(rotation);

            //..........Escala pulsante
            float scaleFactor = 1.0f + std::sin(scaleTime + deltaTime) * scaleSpeed * deltaTime;
            shapePtr->setScale(scaleFactor * scale.x, scaleFactor * scale.y);
            scaleTime += deltaTime;
        }
    }

    void setSize(sf::Vector2f newSize) {
        size = newSize;
        static_cast<sf::RectangleShape*>(shapePtr.get())->setSize(size);
        shapePtr->setOrigin(size.x / 2, size.y / 2);
    }

    sf::Vector2f getSize() const { return size; }

    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    float getRotationSpeed() const { return rotationSpeed; }

    void setScaleSpeed(float speed) { scaleSpeed = speed; }
    float getScaleSpeed() const { return scaleSpeed; }

private:
    sf::Vector2f size;
    float rotationSpeed; //..........Velocidad de rotación
    float scaleSpeed;    //..........Velocidad de cambio de escala
    float scaleTime = 0.0f;
};

//..........Clase para Triángulos
class TriangleShapeClass : public ShapeBase {
public:
    TriangleShapeClass(sf::Vector2f position, sf::Color color, float size = 100.0f)
        : ShapeBase(ShapeType::Triangle, position, color), size(size), rotationSpeed(0.0f) {
        shapePtr = std::make_unique<sf::ConvexShape>(3);
        static_cast<sf::ConvexShape*>(shapePtr.get())->setPoint(0, sf::Vector2f(0.0f, size));
        static_cast<sf::ConvexShape*>(shapePtr.get())->setPoint(1, sf::Vector2f(size / 2, 0.0f));
        static_cast<sf::ConvexShape*>(shapePtr.get())->setPoint(2, sf::Vector2f(size, size));
        shapePtr->setFillColor(color);
        shapePtr->setOrigin(size / 2, size / 2);
        shapePtr->setPosition(position);
    }

    //clonacion:
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<TriangleShapeClass>(position, color, size);
    }
    

    void draw(sf::RenderWindow& window) override {
        shapePtr->setRotation(rotation);
        shapePtr->setScale(scale);
        shapePtr->setFillColor(color);
        window.draw(*shapePtr);
    }

    void updateShape(float deltaTime) override {
        if (isAnimated) {
            //..........Rotación continua
            rotation += rotationSpeed * deltaTime;
            if (rotation > 360.0f) rotation -= 360.0f;
            shapePtr->setRotation(rotation);
        }
    }

    void setSize(float newSize) {
        size = newSize;
        static_cast<sf::ConvexShape*>(shapePtr.get())->setPoint(0, sf::Vector2f(0.0f, size));
        static_cast<sf::ConvexShape*>(shapePtr.get())->setPoint(1, sf::Vector2f(size / 2, 0.0f));
        static_cast<sf::ConvexShape*>(shapePtr.get())->setPoint(2, sf::Vector2f(size, size));
        shapePtr->setOrigin(size / 2, size / 2);
    }

    float getSize() const { return size; }

    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    float getRotationSpeed() const { return rotationSpeed; }

private:
    float size;
    float rotationSpeed; //..........Velocidad de rotación
};

//..........Clase para Elipses
class EllipseShapeClass : public ShapeBase {
public:
    EllipseShapeClass(sf::Vector2f position, sf::Color color, float radiusX = 60.0f, float radiusY = 40.0f)
        : ShapeBase(ShapeType::Ellipse, position, color), radiusX(radiusX), radiusY(radiusY), rotationSpeed(0.0f) {
        shapePtr = std::make_unique<sf::CircleShape>(1.0f, 100); //..........Usar CircleShape con escala para crear una elipse
        shapePtr->setFillColor(color);
        shapePtr->setOrigin(radiusX, radiusY);
        shapePtr->setPosition(position);
        shapePtr->setScale(radiusX / 50.0f, radiusY / 50.0f); //..........Ajustar el radio según el tamaño deseado
    }

    void draw(sf::RenderWindow& window) override {
        shapePtr->setRotation(rotation);
        shapePtr->setScale(scale);
        shapePtr->setFillColor(color);
        window.draw(*shapePtr);
    }

    void updateShape(float deltaTime) override {
        if (isAnimated) {
            //..........Rotación continua
            rotation += rotationSpeed * deltaTime;
            if (rotation > 360.0f) rotation -= 360.0f;
            shapePtr->setRotation(rotation);
        }
    }

    void setRadiusX(float rX) {
        radiusX = rX;
        shapePtr->setScale(radiusX / 50.0f, shapePtr->getScale().y);
        shapePtr->setOrigin(radiusX, radiusY);
    }

    void setRadiusY(float rY) {
        radiusY = rY;
        shapePtr->setScale(shapePtr->getScale().x, radiusY / 50.0f);
        shapePtr->setOrigin(radiusX, radiusY);
    }

    //clonacion sin override:
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<EllipseShapeClass>(position, color, radiusX, radiusY);
    }

    float getRadiusX() const { return radiusX; }
    float getRadiusY() const { return radiusY; }

    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    float getRotationSpeed() const { return rotationSpeed; }

private:
    float radiusX;
    float radiusY;
    float rotationSpeed; //..........Velocidad de rotación
};

//..........Clase para Polígonos
class PolygonShapeClass : public ShapeBase {
public:
    PolygonShapeClass(sf::Vector2f position, sf::Color color, const std::vector<sf::Vector2f>& points)
        : ShapeBase(ShapeType::Polygon, position, color), points(points), rotationSpeed(0.0f) {
        shapePtr = std::make_unique<sf::ConvexShape>(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            static_cast<sf::ConvexShape*>(shapePtr.get())->setPoint(i, points[i]);
        }
        shapePtr->setFillColor(color);
        //..........Calculating centroid for origin
        sf::FloatRect bounds = shapePtr->getLocalBounds();
        shapePtr->setOrigin(bounds.width / 2, bounds.height / 2);
        shapePtr->setPosition(position);
    }

    void draw(sf::RenderWindow& window) override {
        shapePtr->setRotation(rotation);
        shapePtr->setScale(scale);
        shapePtr->setFillColor(color);
        window.draw(*shapePtr);
    }

    void updateShape(float deltaTime) override {
        if (isAnimated) {
            //..........Rotación continua
            rotation += rotationSpeed * deltaTime;
            if (rotation > 360.0f) rotation -= 360.0f;
            shapePtr->setRotation(rotation);
        }
    }

    void setPoints(const std::vector<sf::Vector2f>& newPoints) {
        points = newPoints;
        sf::ConvexShape* convex = static_cast<sf::ConvexShape*>(shapePtr.get());
        convex->setPointCount(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            convex->setPoint(i, points[i]);
        }
        //..........Recalcular el origen
        sf::FloatRect bounds = convex->getLocalBounds();
        convex->setOrigin(bounds.width / 2, bounds.height / 2);
    }

    std::vector<sf::Vector2f> getPoints() const { return points; }

    //clonacion sin override:
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<PolygonShapeClass>(position, color, points);
    }

    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    float getRotationSpeed() const { return rotationSpeed; }

private:
    std::vector<sf::Vector2f> points;
    float rotationSpeed; //..........Velocidad de rotación
};

//..........Clase para Líneas
class LineShapeClass : public ShapeBase {
public:
    LineShapeClass(sf::Vector2f startPoint, sf::Vector2f endPoint, sf::Color color, float thickness = 5.0f)
        : ShapeBase(ShapeType::Line, (startPoint + endPoint) / 2.0f, color), thickness(thickness), rotationSpeed(0.0f) {
        shapePtr = std::make_unique<sf::RectangleShape>();
        sf::Vector2f direction = endPoint - startPoint;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        static_cast<sf::RectangleShape*>(shapePtr.get())->setSize(sf::Vector2f(length, thickness));
        shapePtr->setFillColor(color);
        shapePtr->setPosition(position);
        shapePtr->setOrigin(0, thickness / 2.0f);
        float angle = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
        shapePtr->setRotation(angle);
    }

    void draw(sf::RenderWindow& window) override {
        shapePtr->setRotation(rotation);
        shapePtr->setScale(scale);
        shapePtr->setFillColor(color);
        window.draw(*shapePtr);
    }

    void updateShape(float deltaTime) override {
        if (isAnimated) {
            //..........Rotación continua
            rotation += rotationSpeed * deltaTime;
            if (rotation > 360.0f) rotation -= 360.0f;
            shapePtr->setRotation(rotation);
        }
    }

    void setThickness(float newThickness) {
        thickness = newThickness;
        static_cast<sf::RectangleShape*>(shapePtr.get())->setSize(sf::Vector2f(
            static_cast<sf::RectangleShape*>(shapePtr.get())->getSize().x, thickness));
        shapePtr->setOrigin(0, thickness / 2.0f);
    }

    //clonacion sin override:
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<LineShapeClass>(position, position + sf::Vector2f(100.0f, 0.0f), color, thickness);
    }

    float getThickness() const { return thickness; }

    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    float getRotationSpeed() const { return rotationSpeed; }

private:
    float thickness;
    float rotationSpeed; //..........Velocidad de rotación
};

//..........Clase para la cámara con zoom y movimiento avanzado
class Camera {
public:
    Camera(const sf::Vector2f& position, float zoom)
        : position(position), zoomLevel(zoom), moveSpeed(300.f), zoomSpeed(1.1f), rotation(0.0f), rotationSpeed(50.0f) {}

    void handleInput(const sf::Event& event) {
        //..........Manejo de rotación con teclas E y Q
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::E)
                rotation += rotationSpeed;
            if (event.key.code == sf::Keyboard::Q)
                rotation -= rotationSpeed;
        }
    }

    void update(float deltaTime) {
        sf::Vector2f movement(0.f, 0.f);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            movement.y -= moveSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            movement.y += moveSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            movement.x -= moveSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            movement.x += moveSpeed * deltaTime;

        position += movement;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
            zoomLevel *= std::pow(zoomSpeed, deltaTime * 60); //..........Zoom in
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
            zoomLevel /= std::pow(zoomSpeed, deltaTime * 60); //..........Zoom out
    }

    sf::View getView() const {
        sf::View view;
        view.setCenter(position);
        view.setSize(1280.f / zoomLevel, 720.f / zoomLevel);
        view.setRotation(rotation);
        return view;
    }

private:
    sf::Vector2f position;
    float zoomLevel;
    float moveSpeed;
    float zoomSpeed;
    float rotation;
    float rotationSpeed; //..........Velocidad de rotación
};

//....estructura para acciones (Deshacer/Rehacer)
struct Action {
    enum class Type { Add, Remove, Modify } type;
    std::unique_ptr<ShapeBase> shape;
    int index; //..........Posición en el vector de formas

    //..........Constructores por defecto
    Action() = default;

    //..........Constructor de movimiento
    Action(Action&& other) noexcept = default;

    //..........Operador de asignación por movimiento
    Action& operator=(Action&& other) noexcept = default;

    //..........Eliminar constructor de copia y operador de asignación por copia
    Action(const Action&) = delete;
    Action& operator=(const Action&) = delete;
};


//.....sistema de Deshacer y Rehacer
class UndoRedoManager {
public:
    //..........Añadir una acción moviendo el objeto Action
    void addAction(Action action) {
        undoStack.push(std::move(action));
        //..........Limpiar la pila de rehacer al añadir una nueva acción
        while (!redoStack.empty()) redoStack.pop();
    }

    //..........Verificar si se puede deshacer
    bool canUndo() const { return !undoStack.empty(); }

    //..........Verificar si se puede rehacer
    bool canRedo() const { return !redoStack.empty(); }

    //..........Realizar una acción de deshacer moviendo el objeto Action
    Action undo() {
        if (canUndo()) {
            Action action = std::move(undoStack.top());
            undoStack.pop();
            redoStack.push(std::move(action));
            return action;
        }
        return Action{};
    }

    //..........Realizar una acción de rehacer moviendo el objeto Action
    Action redo() {
        if (canRedo()) {
            Action action = std::move(redoStack.top());
            redoStack.pop();
            undoStack.push(std::move(action));
            return action;
        }
        return Action{};
    }

private:
    std::stack<Action> undoStack;
    std::stack<Action> redoStack;
};

//..........Función para aplicar un estilo moderno y profesional a ImGui
void aplicarEstiloProfesional() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.ScrollbarSize = 15.0f;

    //..........Paleta de colores personalizada
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.35f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.40f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.50f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.42f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.40f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.45f, 0.65f, 1.00f);
    //..........Añadir más colores según necesidad
}

//..........Función para guardar la configuración de la escena
void guardarEscena(const std::string& nombreArchivo, const std::vector<std::unique_ptr<ShapeBase>>& formas) {
    std::ofstream archivo(nombreArchivo);
    if (archivo.is_open()) {
        archivo << formas.size() << "\n";
        for (const auto& forma : formas) {
            archivo << static_cast<int>(forma->getType()) << " ";
            archivo << forma->getPosition().x << " " << forma->getPosition().y << " ";
            archivo << forma->getRotation() << " ";
            archivo << forma->getScale().x << " " << forma->getScale().y << " ";
            archivo << static_cast<int>(forma->getColor().r) << " "
                    << static_cast<int>(forma->getColor().g) << " "
                    << static_cast<int>(forma->getColor().b) << " "
                    << static_cast<int>(forma->getColor().a) << " ";
            archivo << forma->animated() << " "; //..........Indicador de animación

            //..........Escribir propiedades específicas según el tipo
            switch (forma->getType()) {
                case ShapeType::Circle: {
                    CircleShapeClass* circulo = dynamic_cast<CircleShapeClass*>(forma.get());
                    if (circulo) {
                        archivo << circulo->getRadius() << " " << circulo->getRotationSpeed() << " " << circulo->getScaleSpeed();
                    }
                    break;
                }
                case ShapeType::Rectangle: {
                    RectangleShapeClass* rect = dynamic_cast<RectangleShapeClass*>(forma.get());
                    if (rect) {
                        sf::Vector2f size = rect->getSize();
                        archivo << size.x << " " << size.y << " " << rect->getRotationSpeed() << " " << rect->getScaleSpeed();
                    }
                    break;
                }
                case ShapeType::Triangle: {
                    TriangleShapeClass* tri = dynamic_cast<TriangleShapeClass*>(forma.get());
                    if (tri) {
                        archivo << tri->getSize() << " " << tri->getRotationSpeed();
                    }
                    break;
                }
                case ShapeType::Ellipse: {
                    EllipseShapeClass* elip = dynamic_cast<EllipseShapeClass*>(forma.get());
                    if (elip) {
                        archivo << elip->getRadiusX() << " " << elip->getRadiusY() << " " << elip->getRotationSpeed();
                    }
                    break;
                }
                case ShapeType::Polygon: {
                    PolygonShapeClass* poli = dynamic_cast<PolygonShapeClass*>(forma.get());
                    if (poli) {
                        std::vector<sf::Vector2f> puntos = poli->getPoints();
                        archivo << puntos.size();
                        for (const auto& punto : puntos) {
                            archivo << " " << punto.x << " " << punto.y;
                        }
                        archivo << " " << poli->getRotationSpeed();
                    }
                    break;
                }
                case ShapeType::Line: {
                    LineShapeClass* line = dynamic_cast<LineShapeClass*>(forma.get());
                    if (line) {
                        archivo << line->getThickness() << " " << line->getRotationSpeed();
                    }
                    break;
                }
                case ShapeType::Text: {
                    TextShapeClass* texto = dynamic_cast<TextShapeClass*>(forma.get());
                    if (texto) {
                        archivo << texto->getContent() << " " << texto->getCharacterSize() << " ";
                        //..........No hay propiedades de animación específicas para texto en este ejemplo
                    }
                    break;
                }
                default:
                    break;
            }
            archivo << "\n";
        }
        archivo.close();
    }
}

//..........Función para cargar la configuración de la escena
void cargarEscena(const std::string& nombreArchivo, std::vector<std::unique_ptr<ShapeBase>>& formas, const sf::Font& font) {
    std::ifstream archivo(nombreArchivo);
    if (archivo.is_open()) {
        size_t cantidad;
        archivo >> cantidad;
        formas.clear();
        for (size_t i = 0; i < cantidad; ++i) {
            int tipoInt;
            float posX, posY, rot, escX, escY;
            int r, g, b, a;
            bool animado;
            archivo >> tipoInt >> posX >> posY >> rot >> escX >> escY >> r >> g >> b >> a >> animado;

            ShapeType tipo = static_cast<ShapeType>(tipoInt);
            sf::Vector2f posicion(posX, posY);
            sf::Color color(r, g, b, a);
            std::unique_ptr<ShapeBase> nuevaForma = nullptr;

            switch (tipo) {
                case ShapeType::Circle: {
                    float radio, rotSpeed, scaleSpeed;
                    archivo >> radio >> rotSpeed >> scaleSpeed;
                    nuevaForma = std::make_unique<CircleShapeClass>(posicion, color, radio);
                    dynamic_cast<CircleShapeClass*>(nuevaForma.get())->setRotationSpeed(rotSpeed);
                    dynamic_cast<CircleShapeClass*>(nuevaForma.get())->setScaleSpeed(scaleSpeed);
                    break;
                }
                case ShapeType::Rectangle: {
                    float sizeX, sizeY, rotSpeed, scaleSpeed;
                    archivo >> sizeX >> sizeY >> rotSpeed >> scaleSpeed;
                    nuevaForma = std::make_unique<RectangleShapeClass>(posicion, color, sf::Vector2f(sizeX, sizeY));
                    dynamic_cast<RectangleShapeClass*>(nuevaForma.get())->setRotationSpeed(rotSpeed);
                    dynamic_cast<RectangleShapeClass*>(nuevaForma.get())->setScaleSpeed(scaleSpeed);
                    break;
                }
                case ShapeType::Triangle: {
                    float size, rotSpeed;
                    archivo >> size >> rotSpeed;
                    nuevaForma = std::make_unique<TriangleShapeClass>(posicion, color, size);
                    dynamic_cast<TriangleShapeClass*>(nuevaForma.get())->setRotationSpeed(rotSpeed);
                    break;
                }
                case ShapeType::Ellipse: {
                    float rX, rY, rotSpeed;
                    archivo >> rX >> rY >> rotSpeed;
                    nuevaForma = std::make_unique<EllipseShapeClass>(posicion, color, rX, rY);
                    dynamic_cast<EllipseShapeClass*>(nuevaForma.get())->setRotationSpeed(rotSpeed);
                    break;
                }
                case ShapeType::Polygon: {
                    size_t puntosCount;
                    archivo >> puntosCount;
                    std::vector<sf::Vector2f> puntos(puntosCount);
                    for (size_t p = 0; p < puntosCount; ++p) {
                        archivo >> puntos[p].x >> puntos[p].y;
                    }
                    float rotSpeed;
                    archivo >> rotSpeed;
                    nuevaForma = std::make_unique<PolygonShapeClass>(posicion, color, puntos);
                    dynamic_cast<PolygonShapeClass*>(nuevaForma.get())->setRotationSpeed(rotSpeed);
                    break;
                }
                case ShapeType::Line: {
                    float grosor, rotSpeed;
                    archivo >> grosor >> rotSpeed;
                    //..........Para simplificar, asumimos que la línea está horizontal centrada
                    nuevaForma = std::make_unique<LineShapeClass>(
                        sf::Vector2f(posX - 50.0f, posY),
                        sf::Vector2f(posX + 50.0f, posY),
                        color,
                        grosor
                    );
                    dynamic_cast<LineShapeClass*>(nuevaForma.get())->setRotationSpeed(rotSpeed);
                    break;
                }
                case ShapeType::Text: {
                    std::string contenido;
                    unsigned int tam;
                    archivo >> std::ws;
                    std::getline(archivo, contenido, ' '); //..........Leer hasta el espacio
                    archivo >> tam;
                    nuevaForma = std::make_unique<TextShapeClass>(posicion, color, contenido, font);
                    dynamic_cast<TextShapeClass*>(nuevaForma.get())->setCharacterSize(tam);
                    break;
                }
                default:
                    break;
            }

            if (nuevaForma) {
                if (animado)
                    nuevaForma->enableAnimation(true);
                else
                    nuevaForma->enableAnimation(false);
                nuevaForma->setRotation(rot);
                nuevaForma->setScale(sf::Vector2f(escX, escY));
                formas.push_back(std::move(nuevaForma));
            }
        }
        archivo.close();
    }
}

//..........Clase para Anotaciones (Etiquetas de Texto)
class Annotation {
public:
    Annotation(sf::Vector2f position, const std::string& content, const sf::Font& font, unsigned int size = 16)
        : position(position), content(content) {
        text.setFont(font);
        text.setString(content);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color::White);
        text.setPosition(position);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(text);
    }

    void setContent(const std::string& newContent) {
        content = newContent;
        text.setString(content);
    }

    const sf::Vector2f& getPosition() const { return position; }
    void setPosition(const sf::Vector2f& pos) { position = pos; text.setPosition(pos); }

private:
    sf::Vector2f position;
    std::string content;
    sf::Text text;
};

//..........Estructura para conectar formas y crear efecto pseudo-3D
struct Connection {
    int shapeAIndex;
    int shapeBIndex;
};

//..........Función para dibujar conexiones (líneas) entre formas para simular 3D
void drawConnections(sf::RenderWindow& window, const std::vector<std::unique_ptr<ShapeBase>>& formas, const std::vector<Connection>& connections) {
    sf::VertexArray lines(sf::Lines, connections.size() * 2);
    for (size_t i = 0; i < connections.size(); ++i) {
        if (connections[i].shapeAIndex < formas.size() && connections[i].shapeBIndex < formas.size()) {
            sf::Vector2f posA = formas[connections[i].shapeAIndex]->getPosition();
            sf::Vector2f posB = formas[connections[i].shapeBIndex]->getPosition();
            lines[2 * i].position = posA;
            lines[2 * i].color = sf::Color::White;
            lines[2 * i + 1].position = posB;
            lines[2 * i + 1].color = sf::Color::White;
        }
    }
    window.draw(lines);
}

int main() {
    //..........Crear la ventana de SFML
    sf::RenderWindow window(sf::VideoMode(1280, 720), "FigEDIT @FECORO");
    window.setFramerateLimit(60);

    //..........Inicializar ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        return -1;
    }

    //..........Aplicar estilo profesional a ImGui
    aplicarEstiloProfesional();

    //..........Cargar fuente personalizada para textos y anotaciones
    sf::Font font;
    if (!font.loadFromFile("assets/fonts/OpenSans-Regular.ttf")) {
        //..........Manejo de error si no se puede cargar la fuente
        //..........Por defecto, ImGui usará su fuente interna
    }

    //..........Variables para la aplicación
    std::vector<std::unique_ptr<ShapeBase>> formas;
    std::vector<Annotation> anotaciones;
    std::vector<Connection> conexiones;
    int formaSeleccionada = -1;
    bool show_demo_window = false;
    bool show_another_window = false;
    sf::Clock deltaClock;

    //..........Variables de interacción
    bool arrastrando = false;
    int formaArrastrada = -1;
    sf::Vector2f offset;

    //..........Instancia de la cámara
    Camera camara(sf::Vector2f(640.f, 360.f), 1.0f);

    //..........Gestor de Deshacer/Rehacer
    UndoRedoManager undoRedoManager;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            //..........Procesar eventos de ImGui
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }

            //..........Manejar eventos de la cámara
            camara.handleInput(event);

            //..........Manejar clics del ratón para arrastrar formas
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), camara.getView());
                for (int i = formas.size() - 1; i >= 0; --i) { //..........Iterar de atrás hacia adelante
                    sf::FloatRect bounds = formas[i]->getShapePtr()->getGlobalBounds();
                    if (bounds.contains(mousePos)) {
                        arrastrando = true;
                        formaArrastrada = i;
                        offset = formas[i]->getPosition() - mousePos;
                        formaSeleccionada = i;
                        formas[i]->select();
                        //..........Deselect others
                        for (size_t j = 0; j < formas.size(); ++j) {
                            if (j != i)
                                formas[j]->deselect();
                        }
                        break;
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (arrastrando && formaArrastrada != -1) {
                    //..........Añadir acción de mover para Deshacer
                    Action action;
                    action.type = Action::Type::Modify;
                    action.shape = formas[formaArrastrada]->clone();
                    action.index = formaArrastrada;
                    undoRedoManager.addAction(std::move(action));
                }
                arrastrando = false;
                formaArrastrada = -1;
            }

            if (event.type == sf::Event::MouseMoved && arrastrando) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), camara.getView());
                if (formaArrastrada >= 0 && formaArrastrada < formas.size()) {
                    formas[formaArrastrada]->setPosition(mousePos + offset);
                }
            }

            //..........Atajos de teclado para Deshacer y Rehacer
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.control && event.key.code == sf::Keyboard::Z) {
                    if (undoRedoManager.canUndo()) {
                        Action action = undoRedoManager.undo();
                        //..........Implementar lógica de deshacer según el tipo de acción
                        if (action.type == Action::Type::Add) {
                            formas.erase(formas.begin() + action.index);
                        } else if (action.type == Action::Type::Remove) {
                            formas.insert(formas.begin() + action.index, std::move(action.shape));
                        } else if (action.type == Action::Type::Modify) {
                            formas[action.index] = std::move(action.shape);
                        }
                    }
                }
                if (event.key.control && event.key.code == sf::Keyboard::Y) {
                    if (undoRedoManager.canRedo()) {
                        Action action = undoRedoManager.redo();
                        //..........Implementar lógica de rehacer según el tipo de acción
                        if (action.type == Action::Type::Add) {
                            formas.insert(formas.begin() + action.index, std::move(action.shape));
                        } else if (action.type == Action::Type::Remove) {
                            formas.erase(formas.begin() + action.index);
                        } else if (action.type == Action::Type::Modify) {
                            formas[action.index] = std::move(action.shape);
                        }
                    }
                }
            }
        }

        //..........Actualizar la cámara
        float deltaTime = deltaClock.restart().asSeconds();
        camara.update(deltaTime);

        //..........Actualizar ImGui
        ImGui::SFML::Update(window, deltaClock.restart());

        //..........Ventana de Control de Formas
        ImGui::Begin("Control de Formas");

        //..........Botones para añadir formas
        if (ImGui::Button("Añadir Círculo")) {
            formas.emplace_back(std::make_unique<CircleShapeClass>(sf::Vector2f(400.0f, 300.0f), sf::Color::Green));
            //..........Añadir acción para Deshacer
            Action action;
            action.type = Action::Type::Add;
            action.shape = formas.back()->clone(); //..........Usa el método de clonación para crear una copia adecuada
            action.index = formas.size() - 1;
            undoRedoManager.addAction(std::move(action));
        }
        ImGui::SameLine();
        if (ImGui::Button("Añadir Rectángulo")) {
            formas.emplace_back(std::make_unique<RectangleShapeClass>(sf::Vector2f(600.0f, 300.0f), sf::Color::Blue));
            //..........Añadir acción para Deshacer
            Action action;
            action.type = Action::Type::Add;
            action.shape = formas.back()->clone(); //..........Usa el método de clonación para crear una copia adecuada
            action.index = formas.size() - 1;
            undoRedoManager.addAction(std::move(action));
        }
        ImGui::SameLine();
        if (ImGui::Button("Añadir Triángulo")) {
            formas.emplace_back(std::make_unique<TriangleShapeClass>(sf::Vector2f(800.0f, 300.0f), sf::Color::Red));
            //..........Añadir acción para Deshacer
            Action action;
            action.type = Action::Type::Add;
            action.shape = formas.back()->clone(); //..........Usa el método de clonación para crear una copia adecuada
            action.index = formas.size() - 1;
            undoRedoManager.addAction(std::move(action));
        }
        ImGui::SameLine();
        if (ImGui::Button("Añadir Elipse")) {
            formas.emplace_back(std::make_unique<EllipseShapeClass>(sf::Vector2f(500.0f, 400.0f), sf::Color::Magenta, 80.0f, 40.0f));
            //..........Añadir acción para Deshacer
            Action action;
            action.type = Action::Type::Add;
            action.shape = formas.back()->clone(); //..........Usa el método de clonación para crear una copia adecuada
            action.index = formas.size() - 1;
            undoRedoManager.addAction(std::move(action));
        }
        ImGui::SameLine();
        if (ImGui::Button("Añadir Polígono")) {
            formas.emplace_back(std::make_unique<PolygonShapeClass>(sf::Vector2f(700.0f, 400.0f), sf::Color::Cyan, std::vector<sf::Vector2f>{
                sf::Vector2f(0.0f, 60.0f),
                sf::Vector2f(50.0f, 0.0f),
                sf::Vector2f(100.0f, 60.0f),
                sf::Vector2f(75.0f, 120.0f),
                sf::Vector2f(25.0f, 120.0f)
            }));
            //..........Añadir acción para Deshacer
            Action action;
            action.type = Action::Type::Add;
            action.shape = formas.back()->clone(); //..........Usa el método de clonación para crear una copia adecuada
            action.index = formas.size() - 1;
            undoRedoManager.addAction(std::move(action));
        }
        ImGui::SameLine();
        if (ImGui::Button("Añadir Línea")) {
            formas.emplace_back(std::make_unique<LineShapeClass>(
                sf::Vector2f(800.0f, 500.0f),
                sf::Vector2f(900.0f, 600.0f),
                sf::Color::Yellow,
                4.0f
            ));
            //..........Añadir acción para Deshacer
            Action action;
            action.type = Action::Type::Add;
            action.shape = formas.back()->clone(); //..........Usa el método de clonación para crear una copia adecuada
            action.index = formas.size() - 1;
            undoRedoManager.addAction(std::move(action));
        }
        ImGui::SameLine();
        //..........Dentro de la función principal `main`, en la interfaz de ImGui
        if (ImGui::Button("Añadir Cubo")) {
            formas.emplace_back(std::make_unique<CubeShapeClass>(sf::Vector2f(500.0f, 400.0f), sf::Color::Yellow, 100.0f, 50.0f));
            
            //..........Añadir acción para Deshacer
            Action action;
            action.type = Action::Type::Add;
            action.shape = formas.back()->clone(); //..........Usar clone para copiar la forma correctamente
            action.index = formas.size() - 1;
            undoRedoManager.addAction(std::move(action)); //..........Mover la acción
        }

        ImGui::Separator();

        //..........Lista de formas
        if (formas.empty()) {
            ImGui::Text("No hay formas en la escena.");
        }
        else {
            ImGui::Text("Selecciona una forma para editar:");
            if (ImGui::BeginListBox("##FormasList", ImVec2(-FLT_MIN, 150))) {
                for (size_t i = 0; i < formas.size(); ++i) {
                    std::string label = "Forma " + std::to_string(i + 1) + " (" +
                        (formas[i]->getType() == ShapeType::Circle ? "Círculo" :
                         formas[i]->getType() == ShapeType::Rectangle ? "Rectángulo" :
                         formas[i]->getType() == ShapeType::Triangle ? "Triángulo" :
                         formas[i]->getType() == ShapeType::Ellipse ? "Elipse" :
                         formas[i]->getType() == ShapeType::Polygon ? "Polígono" :
                         formas[i]->getType() == ShapeType::Line ? "Línea" :
                         formas[i]->getType() == ShapeType::Text ? "Texto" : "Desconocido") + ")";
                    bool is_selected = (formaSeleccionada == static_cast<int>(i));
                    if (ImGui::Selectable(label.c_str(), is_selected)) {
                        formaSeleccionada = static_cast<int>(i);
                        //..........Seleccionar la forma en la ventana gráfica
                        formas[i]->select();
                        //..........Deselect others
                        for (size_t j = 0; j < formas.size(); ++j) {
                            if (j != i)
                                formas[j]->deselect();
                        }
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            ImGui::Separator();

            //..........Propiedades de la forma seleccionada
            if (formaSeleccionada >= 0 && formaSeleccionada < static_cast<int>(formas.size())) {
                ImGui::Text("Editar Propiedades de la Forma %d", formaSeleccionada + 1);
                ImGui::Separator();

                //..........Tipo de forma (solo lectura para simplificar)
                std::string tipoForma = "";
                switch (formas[formaSeleccionada]->getType()) {
                    case ShapeType::Circle: tipoForma = "Círculo"; break;
                    case ShapeType::Rectangle: tipoForma = "Rectángulo"; break;
                    case ShapeType::Triangle: tipoForma = "Triángulo"; break;
                    case ShapeType::Ellipse: tipoForma = "Elipse"; break;
                    case ShapeType::Polygon: tipoForma = "Polígono"; break;
                    case ShapeType::Line: tipoForma = "Línea"; break;
                    case ShapeType::Text: tipoForma = "Texto"; break;
                    default: tipoForma = "Desconocido"; break;
                }
                ImGui::Text("Tipo: %s", tipoForma.c_str());

                //..........Posición
                sf::Vector2f pos = formas[formaSeleccionada]->getPosition();
                float posicion[2] = { pos.x, pos.y };
                if (ImGui::SliderFloat2("Posición", posicion, 0.0f, 1280.0f)) {
                    formas[formaSeleccionada]->setPosition(sf::Vector2f(posicion[0], posicion[1]));
                }

                //..........Rotación
                float rotacion = formas[formaSeleccionada]->getRotation();
                if (ImGui::SliderFloat("Rotación [°]", &rotacion, 0.0f, 360.0f)) {
                    formas[formaSeleccionada]->setRotation(rotacion);
                }

                //..........Escala
                sf::Vector2f esc = formas[formaSeleccionada]->getScale();
                float escala[2] = { esc.x, esc.y };
                if (ImGui::SliderFloat2("Escala", escala, 0.1f, 3.0f)) {
                    formas[formaSeleccionada]->setScale(sf::Vector2f(escala[0], escala[1]));
                }

                //..........Color
                sf::Color color = formas[formaSeleccionada]->getColor();
                float colorRGB[4] = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
                if (ImGui::ColorEdit4("Color", colorRGB)) {
                    formas[formaSeleccionada]->setColor(sf::Color(
                        static_cast<sf::Uint8>(colorRGB[0] * 255),
                        static_cast<sf::Uint8>(colorRGB[1] * 255),
                        static_cast<sf::Uint8>(colorRGB[2] * 255),
                        static_cast<sf::Uint8>(colorRGB[3] * 255)
                    ));
                }

                //..........Animación
                bool animado = formas[formaSeleccionada]->animated();
                if (ImGui::Checkbox("Animar", &animado)) {
                    formas[formaSeleccionada]->enableAnimation(animado);
                }

                //..........Propiedades específicas según el tipo de forma
                switch (formas[formaSeleccionada]->getType()) {
                    case ShapeType::Circle: {
                        CircleShapeClass* circulo = dynamic_cast<CircleShapeClass*>(formas[formaSeleccionada].get());
                        if (circulo) {
                            float radio = circulo->getRadius();
                            if (ImGui::SliderFloat("Radio", &radio, 10.0f, 200.0f)) {
                                circulo->setRadius(radio);
                            }
                            float rotSpeed = circulo->getRotationSpeed();
                            if (ImGui::SliderFloat("Velocidad de Rotación", &rotSpeed, 0.0f, 360.0f)) {
                                circulo->setRotationSpeed(rotSpeed);
                            }
                            float scaleSpeed = circulo->getScaleSpeed();
                            if (ImGui::SliderFloat("Velocidad de Escala", &scaleSpeed, 0.0f, 5.0f)) {
                                circulo->setScaleSpeed(scaleSpeed);
                            }
                        }
                        break;
                    }
                    case ShapeType::Rectangle: {
                        RectangleShapeClass* rect = dynamic_cast<RectangleShapeClass*>(formas[formaSeleccionada].get());
                        if (rect) {
                            sf::Vector2f size = rect->getSize();
                            float tamanos[2] = { size.x, size.y };
                            if (ImGui::SliderFloat2("Tamaño", tamanos, 10.0f, 300.0f)) {
                                rect->setSize(sf::Vector2f(tamanos[0], tamanos[1]));
                            }
                            float rotSpeed = rect->getRotationSpeed();
                            if (ImGui::SliderFloat("Velocidad de Rotación", &rotSpeed, 0.0f, 360.0f)) {
                                rect->setRotationSpeed(rotSpeed);
                            }
                            float scaleSpeed = rect->getScaleSpeed();
                            if (ImGui::SliderFloat("Velocidad de Escala", &scaleSpeed, 0.0f, 5.0f)) {
                                rect->setScaleSpeed(scaleSpeed);
                            }
                        }
                        break;
                    }
                    case ShapeType::Triangle: {
                        TriangleShapeClass* tri = dynamic_cast<TriangleShapeClass*>(formas[formaSeleccionada].get());
                        if (tri) {
                            float tam = tri->getSize();
                            if (ImGui::SliderFloat("Tamaño", &tam, 10.0f, 200.0f)) {
                                tri->setSize(tam);
                            }
                            float rotSpeed = tri->getRotationSpeed();
                            if (ImGui::SliderFloat("Velocidad de Rotación", &rotSpeed, 0.0f, 360.0f)) {
                                tri->setRotationSpeed(rotSpeed);
                            }
                        }
                        break;
                    }
                    case ShapeType::Ellipse: {
                        EllipseShapeClass* elip = dynamic_cast<EllipseShapeClass*>(formas[formaSeleccionada].get());
                        if (elip) {
                            float radioX = elip->getRadiusX();
                            float radioY = elip->getRadiusY();
                            if (ImGui::SliderFloat("Radio X", &radioX, 10.0f, 300.0f)) {
                                elip->setRadiusX(radioX);
                            }
                            if (ImGui::SliderFloat("Radio Y", &radioY, 10.0f, 300.0f)) {
                                elip->setRadiusY(radioY);
                            }
                            float rotSpeed = elip->getRotationSpeed();
                            if (ImGui::SliderFloat("Velocidad de Rotación", &rotSpeed, 0.0f, 360.0f)) {
                                elip->setRotationSpeed(rotSpeed);
                            }
                        }
                        break;
                    }
                    case ShapeType::Polygon: {
                        PolygonShapeClass* poli = dynamic_cast<PolygonShapeClass*>(formas[formaSeleccionada].get());
                        if (poli) {
                            //..........Modificar puntos del polígono
                            std::vector<sf::Vector2f> puntos = poli->getPoints();
                            for (size_t p = 0; p < puntos.size(); ++p) {
                                float punto[2] = { puntos[p].x, puntos[p].y };
                                std::string label = "Punto " + std::to_string(p + 1);
                                if (ImGui::SliderFloat2(label.c_str(), punto, -200.0f, 200.0f)) {
                                    puntos[p] = sf::Vector2f(punto[0], punto[1]);
                                    poli->setPoints(puntos);
                                }
                            }
                            float rotSpeed = poli->getRotationSpeed();
                            if (ImGui::SliderFloat("Velocidad de Rotación", &rotSpeed, 0.0f, 360.0f)) {
                                poli->setRotationSpeed(rotSpeed);
                            }
                        }
                        break;
                    }
                    case ShapeType::Line: {
                        LineShapeClass* line = dynamic_cast<LineShapeClass*>(formas[formaSeleccionada].get());
                        if (line) {
                            float grosor = line->getThickness();
                            if (ImGui::SliderFloat("Grosor", &grosor, 1.0f, 20.0f)) {
                                line->setThickness(grosor);
                            }
                            float rotSpeed = line->getRotationSpeed();
                            if (ImGui::SliderFloat("Velocidad de Rotación", &rotSpeed, 0.0f, 360.0f)) {
                                line->setRotationSpeed(rotSpeed);
                            }
                        }
                        break;
                    }
                    case ShapeType::Text: {
                        TextShapeClass* texto = dynamic_cast<TextShapeClass*>(formas[formaSeleccionada].get());
                        if (texto) {
                            char buffer[128];
                            strncpy(buffer, texto->getContent().c_str(), sizeof(buffer));
                            buffer[sizeof(buffer) - 1] = '\0';
                            if (ImGui::InputText("Contenido", buffer, sizeof(buffer))) {
                                texto->setContent(std::string(buffer));
                            }
                            unsigned int tam = texto->getCharacterSize();
                            if (ImGui::SliderInt("Tamaño de Caracteres", reinterpret_cast<int*>(&tam), 8, 72)) {
                                texto->setCharacterSize(tam);
                            }
                            float rotSpeed = 0.0f; //..........No hay rotación específica para texto en este ejemplo
                            ImGui::Text("Rotación automática no disponible para Textos.");
                        }
                        break;
                    }
                    default:
                        break;
                }

                //..........Añadir anotación
                if (ImGui::Button("Añadir Anotación")) {
                    std::string contenido = "Etiqueta " + std::to_string(anotaciones.size() + 1);
                    anotaciones.emplace_back(pos, contenido, font, 16);
                }

                //....botón para eliminar la forma
                if (ImGui::Button("Eliminar Forma")) {
                    //..........Añadir acción para Deshacer
                    Action action;
                    action.type = Action::Type::Remove;
                    action.shape = formas[formaSeleccionada]->clone();
                    action.index = formaSeleccionada;
                    undoRedoManager.addAction(std::move(action));

                    formas.erase(formas.begin() + formaSeleccionada);
                    formaSeleccionada = -1;
                }
            }
        }

        ImGui::End(); //..........Fin de la ventana de Control de Formas

        //..........Ventana de Opciones
        ImGui::Begin("Opciones");

        //..........Botones para guardar y cargar escena
        if (ImGui::Button("Guardar Escena")) {
            guardarEscena("escena.txt", formas);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cargar Escena")) {
            cargarEscena("escena.txt", formas, font);
        }

        ImGui::Separator();

        //..........Control de cámara avanzado
        ImGui::Text("Control de Cámara:");
        ImGui::Text("Movimiento: W/A/S/D");
        ImGui::Text("Rotación: Q/E");
        ImGui::Text("Zoom: Z (acercar), X (alejar)");

        //..........Opciones adicionales
        if (ImGui::Button("Deshacer")) {
            if (undoRedoManager.canUndo()) {
                Action action = undoRedoManager.undo();
                //..........Implementar lógica de deshacer según el tipo de acción
                if (action.type == Action::Type::Add) {
                    formas.erase(formas.begin() + action.index);
                }
                else if (action.type == Action::Type::Remove) {
                    formas.insert(formas.begin() + action.index, std::move(action.shape));
                }
                else if (action.type == Action::Type::Modify) {
                    formas[action.index] = std::move(action.shape);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Rehacer")) {
            if (undoRedoManager.canRedo()) {
                Action action = undoRedoManager.redo();
                //..........Implementar lógica de rehacer según el tipo de acción
                if (action.type == Action::Type::Add) {
                    formas.insert(formas.begin() + action.index, std::move(action.shape));
                }
                else if (action.type == Action::Type::Remove) {
                    formas.erase(formas.begin() + action.index);
                }
                else if (action.type == Action::Type::Modify) {
                    formas[action.index] = std::move(action.shape);
                }
            }
        }

        ImGui::Separator();

        ImGui::End(); //..........Fin de la ventana de Opciones

        //..........Ventana de Anotaciones
        ImGui::Begin("Anotaciones");
        for (size_t i = 0; i < anotaciones.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            std::string label = "Anotación " + std::to_string(i + 1);
            if (ImGui::TreeNode(label.c_str())) {
                char buffer[128];
                //strncpy(buffer, anotaciones[i].getPosition().x, sizeof(buffer));
                //argument of type "float" is incompatible with parameter of type "const char *":
                strncpy(buffer, "", sizeof(buffer)); //..........Placeholder
                snprintf(buffer, sizeof(buffer), "%.1f, %.1f", anotaciones[i].getPosition().x, anotaciones[i].getPosition().y);
                ImGui::Text("Posición: %s", buffer);
                char contentBuffer[128];
                strncpy(contentBuffer, "", sizeof(contentBuffer)); //..........Placeholder
                //..........Implementar edición de contenido si es necesario
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::End();

        //..........Mostrar ventana demo de ImGui si está activada
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        //..........Renderizar
        window.setView(camara.getView());
        window.clear(sf::Color(20, 20, 30)); //..........Fondo oscuro

        //..........Dibujar conexiones para efecto pseudo-3D
        drawConnections(window, formas, conexiones);

        //..........Dibujar todas las formas
        for (auto& forma : formas) {
            forma->draw(window);
        }

        //..........Dibujar anotaciones
        for (const auto& anotacion : anotaciones) {
            anotacion.draw(window);
        }

        //..........Actualizar animaciones
        for (auto& forma : formas) {
            forma->updateShape(deltaTime);
        }

        //..........Renderizar la interfaz de ImGui
        ImGui::SFML::Render(window);
        window.display();
    }

    //..........Finalizar ImGui-SFML
    ImGui::SFML::Shutdown();

    return 0;
}

//..........Fin del archivo main.cpp
