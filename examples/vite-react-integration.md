# ZenKit + Vite React Integration Guide

## 🚀 Method 1: Vite Native WASM Support (Recommended)

### 1. **Install ZenKit Package**
```bash
npm install @kolarz3/zenkit
```

### 2. **Vite Configuration**
```javascript
// vite.config.js
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  optimizeDeps: {
    exclude: ['@kolarz3/zenkit'] // Prevent pre-bundling
  },
  server: {
    fs: {
      allow: ['..'] // Allow access to parent directories if needed
    }
  }
})
```

### 3. **React Hook for ZenKit**
```javascript
// hooks/useZenKit.js
import { useState, useEffect, useCallback } from 'react'

export function useZenKit() {
  const [zenKit, setZenKit] = useState(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)

  useEffect(() => {
    let mounted = true

    async function initZenKit() {
      try {
        setLoading(true)
        
        // Dynamic import with proper error handling
        const ZenKitModule = (await import('@kolarz3/zenkit')).default
        const zenKitInstance = await ZenKitModule()
        
        if (mounted) {
          setZenKit(zenKitInstance)
          setError(null)
        }
      } catch (err) {
        console.error('Failed to initialize ZenKit:', err)
        if (mounted) {
          setError(err.message)
        }
      } finally {
        if (mounted) {
          setLoading(false)
        }
      }
    }

    initZenKit()

    return () => {
      mounted = false
    }
  }, [])

  const loadWorldFromFile = useCallback(async (file) => {
    if (!zenKit) throw new Error('ZenKit not initialized')
    
    const arrayBuffer = await file.arrayBuffer()
    const uint8Array = new Uint8Array(arrayBuffer)
    
    const world = zenKit.createWorld()
    const success = world.loadFromArray(uint8Array)
    
    if (!success || !world.isLoaded) {
      throw new Error(world.getLastError() || 'Failed to load world')
    }
    
    return world
  }, [zenKit])

  const loadWorldFromUrl = useCallback(async (url) => {
    if (!zenKit) throw new Error('ZenKit not initialized')
    
    const response = await fetch(url)
    if (!response.ok) throw new Error(`Failed to fetch ${url}`)
    
    const arrayBuffer = await response.arrayBuffer()
    const uint8Array = new Uint8Array(arrayBuffer)
    
    const world = zenKit.createWorld()
    const success = world.loadFromArray(uint8Array)
    
    if (!success || !world.isLoaded) {
      throw new Error(world.getLastError() || 'Failed to load world')
    }
    
    return world
  }, [zenKit])

  return {
    zenKit,
    loading,
    error,
    loadWorldFromFile,
    loadWorldFromUrl
  }
}
```

### 4. **React Component Example**
```javascript
// components/ZenFileViewer.jsx
import React, { useState } from 'react'
import { useZenKit } from '../hooks/useZenKit'

export function ZenFileViewer() {
  const { zenKit, loading, error, loadWorldFromFile } = useZenKit()
  const [world, setWorld] = useState(null)
  const [meshInfo, setMeshInfo] = useState(null)
  const [loadingFile, setLoadingFile] = useState(false)

  const handleFileSelect = async (event) => {
    const file = event.target.files[0]
    if (!file) return

    try {
      setLoadingFile(true)
      const loadedWorld = await loadWorldFromFile(file)
      setWorld(loadedWorld)
      
      // Extract mesh info
      const mesh = loadedWorld.mesh
      setMeshInfo({
        vertices: mesh.vertexCount,
        features: mesh.featureCount,
        indices: mesh.indexCount,
        materials: mesh.materials.size(),
        boundingBox: {
          min: mesh.boundingBoxMin,
          max: mesh.boundingBoxMax
        }
      })
    } catch (err) {
      console.error('Failed to load file:', err)
      alert(`Failed to load file: ${err.message}`)
    } finally {
      setLoadingFile(false)
    }
  }

  if (loading) {
    return <div>Loading ZenKit...</div>
  }

  if (error) {
    return <div>Error: {error}</div>
  }

  return (
    <div className="zen-viewer">
      <h2>ZenKit File Viewer</h2>
      
      <input 
        type="file" 
        accept=".zen" 
        onChange={handleFileSelect}
        disabled={loadingFile}
      />
      
      {loadingFile && <div>Loading file...</div>}
      
      {meshInfo && (
        <div className="mesh-info">
          <h3>Mesh Information</h3>
          <p>Vertices: {meshInfo.vertices.toLocaleString()}</p>
          <p>Features: {meshInfo.features.toLocaleString()}</p>
          <p>Indices: {meshInfo.indices.toLocaleString()}</p>
          <p>Materials: {meshInfo.materials}</p>
          <p>Bounding Box: 
            ({meshInfo.boundingBox.min.x.toFixed(1)}, {meshInfo.boundingBox.min.y.toFixed(1)}, {meshInfo.boundingBox.min.z.toFixed(1)}) - 
            ({meshInfo.boundingBox.max.x.toFixed(1)}, {meshInfo.boundingBox.max.y.toFixed(1)}, {meshInfo.boundingBox.max.z.toFixed(1)})
          </p>
        </div>
      )}
    </div>
  )
}
```

---

## 🔧 Method 2: Using vite-plugin-wasm

### 1. **Install Plugin**
```bash
npm install -D vite-plugin-wasm
npm install @kolarz3/zenkit
```

### 2. **Vite Configuration**
```javascript
// vite.config.js
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import wasm from 'vite-plugin-wasm'

export default defineConfig({
  plugins: [
    react(),
    wasm()
  ],
  optimizeDeps: {
    exclude: ['@kolarz3/zenkit']
  }
})
```

### 3. **Usage**
```javascript
// Same as Method 1, but with enhanced WASM support
import { useZenKit } from './hooks/useZenKit'
```

---

## 📦 Method 3: Manual Asset Management (Fallback)

### 1. **Copy WASM Files to Public**
```bash
# Add to package.json scripts
{
  "scripts": {
    "prepare-wasm": "cp node_modules/@kolarz3/zenkit/*.wasm public/",
    "dev": "npm run prepare-wasm && vite",
    "build": "npm run prepare-wasm && vite build"
  }
}
```

### 2. **Custom Loader**
```javascript
// utils/zenkit-loader.js
export async function loadZenKit() {
  // Try to load from CDN/node_modules first
  try {
    const ZenKitModule = (await import('@kolarz3/zenkit')).default
    return await ZenKitModule()
  } catch (error) {
    console.warn('Failed to load from package, trying public folder...', error)
    
    // Fallback to public folder
    const ZenKitModule = (await import('/zenkit.mjs')).default
    return await ZenKitModule({
      locateFile: (path) => {
        if (path.endsWith('.wasm')) {
          return `/zenkit.wasm`
        }
        return path
      }
    })
  }
}
```

---

## 🎯 Best Practices

### 1. **Error Boundaries**
```javascript
// components/ZenKitErrorBoundary.jsx
import React from 'react'

export class ZenKitErrorBoundary extends React.Component {
  constructor(props) {
    super(props)
    this.state = { hasError: false, error: null }
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error }
  }

  componentDidCatch(error, errorInfo) {
    console.error('ZenKit Error:', error, errorInfo)
  }

  render() {
    if (this.state.hasError) {
      return (
        <div className="error-boundary">
          <h2>Something went wrong with ZenKit</h2>
          <details>
            <summary>Error details</summary>
            <pre>{this.state.error?.toString()}</pre>
          </details>
          <button onClick={() => this.setState({ hasError: false, error: null })}>
            Try again
          </button>
        </div>
      )
    }

    return this.props.children
  }
}
```

### 2. **Environment Detection**
```javascript
// utils/environment.js
export const isProduction = import.meta.env.PROD
export const isDevelopment = import.meta.env.DEV

export function getZenKitPath() {
  if (isDevelopment) {
    // In development, try node_modules first
    return '@kolarz3/zenkit'
  } else {
    // In production, might need to adjust based on deployment
    return '@kolarz3/zenkit'
  }
}
```

### 3. **Performance Optimization**
```javascript
// hooks/useZenKitWorker.js (for heavy operations)
import { useState, useEffect } from 'react'

export function useZenKitWorker() {
  const [worker, setWorker] = useState(null)

  useEffect(() => {
    const zenKitWorker = new Worker(
      new URL('../workers/zenkit-worker.js', import.meta.url),
      { type: 'module' }
    )
    
    setWorker(zenKitWorker)
    
    return () => {
      zenKitWorker.terminate()
    }
  }, [])

  return worker
}
```

---

## 🚀 Complete Example App.jsx

```javascript
import React from 'react'
import { ZenFileViewer } from './components/ZenFileViewer'
import { ZenKitErrorBoundary } from './components/ZenKitErrorBoundary'
import './App.css'

function App() {
  return (
    <div className="App">
      <header className="App-header">
        <h1>Gothic World Viewer</h1>
      </header>
      
      <main>
        <ZenKitErrorBoundary>
          <ZenFileViewer />
        </ZenKitErrorBoundary>
      </main>
    </div>
  )
}

export default App
```

---

## 🎯 Recommended Approach

**Use Method 1** (Vite Native WASM Support) as it's:
- ✅ Most reliable
- ✅ No additional plugins needed
- ✅ Works with npm packages
- ✅ Proper asset handling
- ✅ Good development experience

The key is proper `vite.config.js` setup and using dynamic imports with error handling!

