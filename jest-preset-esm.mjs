/**
 * Jest preset for ES modules with real WASM loading
 */

export default {
  // Use babel to transform all .mjs files except the WASM module
  transform: {
    '^.+\\.mjs$': 'babel-jest'
  },
  
  // Don't transform the WASM module or any .mjs files except tests
  transformIgnorePatterns: [
    'node_modules/(?!(.*\\.mjs$))',
    'build-wasm/.*'
  ],

  // Module file extensions
  moduleFileExtensions: ['mjs', 'js', 'json'],



  // Test environment
  testEnvironment: 'node',

  // Global setup
  globals: {
    '__DEV__': true
  }
};
