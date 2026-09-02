uniform sampler2D texture;
uniform vec2 texelSize;
uniform vec2 direction;

void main() {
    vec2 uv = gl_TexCoord[0].xy;

    vec4 result = texture2D(texture, uv) * 0.2270270270;
    result += texture2D(texture, uv + texelSize * direction * 1.3846153846) * 0.3162162162;
    result += texture2D(texture, uv - texelSize * direction * 1.3846153846) * 0.3162162162;
    result += texture2D(texture, uv + texelSize * direction * 3.2307692308) * 0.0702702703;
    result += texture2D(texture, uv - texelSize * direction * 3.2307692308) * 0.0702702703;

    gl_FragColor = result;
}
