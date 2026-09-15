// ESLint 9의 flat config다. 이 파일이 없으면 eslint가 설정을 찾지 못해 실행 자체가 실패한다.
import js from '@eslint/js';
import globals from 'globals';

export default [
    // 빌드 산출물과 의존성은 검사하지 않는다. obj/는 .esproj의 NuGet 복원 산출물이다.
    { ignores: ['node_modules/**', 'obj/**'] },

    js.configs.recommended,

    {
        files: ['**/*.js'],
        languageOptions: {
            ecmaVersion: 'latest',
            // package.json의 "type": "module"과 맞춘다.
            sourceType: 'module',
            globals: {
                ...globals.node,
            },
        },
    },
];
