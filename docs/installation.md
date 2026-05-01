# Installation Guide

## System Requirements
- Windows, macOS, or Linux
- GCC compiler (version 5.0+)
- SQLite3 development libraries
- Modern web browser for frontend

## Prerequisites

### On Windows
```powershell
# Install MinGW (includes GCC)
choco install mingw
# Or download from: https://sourceforge.net/projects/mingw-w64/

# Install SQLite3 development library
choco install sqlite
```

### On macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install SQLite3 (usually pre-installed)
brew install sqlite3
```

### On Linux (Ubuntu/Debian)
```bash
# Install build essentials and SQLite3
sudo apt-get update
sudo apt-get install build-essential sqlite3 libsqlite3-dev
```

## Building the Project

### Step 1: Clone or Navigate to the Project
```bash
cd HireMe-AI
```

### Step 2: Build the Application
```bash
make
```

This will compile all C files in the `backend-c/` directory and create the executable.

### Step 3: Run Tests (Optional)
```bash
make test
```

### Step 4: Run the Application
```bash
./hireme
```

## Running the Frontend

### Option 1: Local Development Server
Use any local web server:

```bash
# Using Python 3
python -m http.server 8000

# Using Node.js (http-server)
npx http-server

# Using PHP
php -S localhost:8000
```

Then open `http://localhost:8000` in your browser.

### Option 2: Direct File Access
Open `frontend/index.html` directly in your web browser (limited functionality for API calls).

## Troubleshooting

### Issue: `fatal error: sqlite3.h: No such file or directory`
**Solution**: Install SQLite3 development headers
- **Windows**: `choco install sqlite`
- **macOS**: `brew install sqlite3`
- **Linux**: `sudo apt-get install libsqlite3-dev`

### Issue: `gcc: command not found`
**Solution**: Install GCC compiler
- **Windows**: Install MinGW-w64
- **macOS**: `xcode-select --install`
- **Linux**: `sudo apt-get install build-essential`

### Issue: Frontend not connecting to backend
**Solution**: Ensure backend server is running on the correct port (check api.c configuration)

## Next Steps
- See [README.md](../README.md) for usage instructions
- Check [architecture.md](./architecture.md) for technical details
- Review backend-c source code for implementation details
