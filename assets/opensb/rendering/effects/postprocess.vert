#version 140

uniform vec2 screenSize;
uniform mat3 vertexTransform;

in vec2 vertexPosition;
in vec2 vertexTextureCoordinate;
in vec4 vertexColor;
in int vertexData;

out vec2 fragmentTextureCoordinate;

void main() {
  vec2 screenPosition = (vertexTransform * vec3(vertexPosition, 1.0)).xy;
  gl_Position = vec4(screenPosition / screenSize * 2.0, 0.0, 1.0);
  fragmentTextureCoordinate = screenPosition / screenSize + 0.5;
}
