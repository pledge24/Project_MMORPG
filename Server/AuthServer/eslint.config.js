// ESLint 9의 flat config다. 이 파일이 없으면 eslint가 설정을 찾지 못해 실행 자체가 실패한다.
import js from '@eslint/js';
import globals from 'globals';

export default [
    // 의존성은 검사하지 않는다.
    { ignores: ['node_modules/**'] },

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
