# SecureChat Qt Client

A modern, secure Qt-based GUI client for the SecureChat real-time messaging system.

## Features

### 🔐 Security
- **End-to-end encryption** with AES-256 and RSA-2048
- **Perfect forward secrecy** with automatic key rotation
- **Message integrity** verification with HMAC
- **Secure authentication** with JWT tokens
- **TLS transport security** for all communications

### 💬 Real-time Messaging
- **Instant messaging** with sub-50ms latency
- **Typing indicators** to show when users are typing
- **Message status tracking** (sent, delivered, read)
- **File sharing** with drag-and-drop support
- **Image preview** and inline display
- **Message history** with search functionality

### 👥 User Management
- **Online user list** with status indicators
- **User presence** (online, away, busy, offline)
- **Private messaging** support
- **User blocking/unblocking** functionality
- **Avatar support** with custom images

### 🎨 Modern UI/UX
- **Dark and light themes** with auto-detection
- **Responsive design** that adapts to window size
- **System tray integration** with notifications
- **Customizable interface** with settings panel
- **Accessibility support** with keyboard navigation
- **Multi-language support** with translations

### ⚡ Performance
- **Asynchronous networking** for smooth UI
- **Efficient message rendering** with virtual scrolling
- **Memory optimization** for large chat histories
- **Background processing** for file transfers
- **Smart caching** for improved responsiveness

## Requirements

### System Requirements
- **Operating System**: Windows 10+, macOS 10.14+, or Linux (Ubuntu 18.04+)
- **Memory**: 512 MB RAM minimum, 1 GB recommended
- **Storage**: 100 MB free space
- **Network**: Internet connection for server communication

### Development Requirements
- **Qt**: Version 5.15+ or 6.2+ (Qt 6 recommended)
- **Compiler**: GCC 10+, Clang 12+, or MSVC 2019+
- **CMake**: Version 3.20 or later
- **OpenSSL**: Version 1.1.1 or later

## Building

### Quick Start
```bash
# Clone the repository
git clone <repository-url>
cd securechat/client/qt

# Install dependencies (Ubuntu/Debian)
sudo apt-get install qt6-base-dev qt6-tools-dev libssl-dev cmake build-essential

# Build the client
./build-client.sh

# Or build manually
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build Options
```bash
# Debug build with tests
./build-client.sh -t Debug --enable-tests

# Release build with installation
./build-client.sh -t Release --install

# Clean build
./build-client.sh --clean

# Check Qt installation
./build-client.sh --check-qt

# Install dependencies
./build-client.sh --deps
```

### Platform-Specific Instructions

#### Windows
1. Install Qt from https://www.qt.io/download
2. Install Visual Studio 2019 or later
3. Install OpenSSL (vcpkg recommended)
4. Use Qt Creator or Visual Studio to build

#### macOS
```bash
# Install dependencies with Homebrew
brew install qt@6 cmake openssl

# Build
./build-client.sh --qt-version 6
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install qt6-base-dev qt6-tools-dev libssl-dev

# Fedora/CentOS
sudo dnf install qt6-qtbase-devel qt6-qttools-devel openssl-devel

# Arch Linux
sudo pacman -S qt6-base qt6-tools openssl
```

## Usage

### First Run
1. Launch the application
2. Enter server connection details (host, port)
3. Create an account or login with existing credentials
4. Start chatting securely!

### Configuration
The client stores settings in:
- **Windows**: `%APPDATA%/SecureChat/`
- **macOS**: `~/Library/Preferences/SecureChat/`
- **Linux**: `~/.config/SecureChat/`

### Command Line Options
```bash
securechat-client [OPTIONS]

Options:
  -s, --server ADDRESS    Server address (default: localhost)
  -p, --port PORT        Server port (default: 8080)
  -t, --theme THEME      Theme (light|dark|auto)
  -d, --debug            Enable debug logging
  --no-tray              Disable system tray
  -h, --help             Show help
  -v, --version          Show version
```

## Architecture

### Component Overview
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Main Window   │────│  Chat Widget    │────│ Message Widget  │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ User List Widget│    │ Login Dialog    │    │Settings Dialog  │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│Client Connection│────│Message Handler  │────│Encryption Client│
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Key Classes
- **MainWindow**: Main application window and coordinator
- **LoginDialog**: Secure authentication interface
- **ChatWidget**: Message display and input handling
- **UserListWidget**: Online users and presence management
- **MessageWidget**: Individual message rendering
- **ClientConnection**: Network communication and protocol handling
- **EncryptionClient**: Client-side encryption and key management
- **ThemeManager**: UI theming and styling
- **ClientLogger**: Logging and debugging support

## Security

### Encryption Details
- **Message Encryption**: AES-256-GCM with random IV per message
- **Key Exchange**: RSA-2048 with OAEP padding
- **Key Derivation**: PBKDF2 with 100,000 iterations
- **Message Authentication**: HMAC-SHA256
- **Transport Security**: TLS 1.3 with certificate validation

### Security Best Practices
- Passwords are never stored locally
- Session keys are rotated every 30 minutes
- All sensitive data is cleared from memory
- Certificate pinning prevents MITM attacks
- Replay protection with sequence numbers

## Customization

### Themes
Create custom themes by modifying the QSS files in `resources/themes/`:
- `light.qss` - Light theme styles
- `dark.qss` - Dark theme styles

### Translations
Add new languages by creating `.ts` files and using Qt Linguist:
```bash
lupdate securechat-client.pro
linguist translations/securechat_xx.ts
lrelease translations/securechat_xx.ts
```

### Plugins
The client supports plugins for extending functionality:
```cpp
class CustomPlugin : public QObject, public ClientPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.securechat.ClientPlugin")
    Q_INTERFACES(ClientPlugin)
    
public:
    void initialize(MainWindow *mainWindow) override;
    void shutdown() override;
};
```

## Troubleshooting

### Common Issues

#### Connection Problems
- Check server address and port
- Verify firewall settings
- Ensure TLS certificates are valid
- Check network connectivity

#### Build Issues
- Verify Qt installation with `./build-client.sh --check-qt`
- Install missing dependencies with `./build-client.sh --deps`
- Check compiler version compatibility
- Clear build directory and rebuild

#### Runtime Issues
- Check log files in the application data directory
- Run with `--debug` flag for verbose logging
- Verify OpenSSL installation
- Check system tray availability

### Debug Mode
Enable debug logging for troubleshooting:
```bash
securechat-client --debug
```

Log files are stored in:
- **Windows**: `%APPDATA%/SecureChat/logs/`
- **macOS**: `~/Library/Logs/SecureChat/`
- **Linux**: `~/.local/share/SecureChat/logs/`

## Contributing

### Development Setup
1. Fork the repository
2. Install development dependencies
3. Create a feature branch
4. Make changes and add tests
5. Submit a pull request

### Code Style
- Follow Qt coding conventions
- Use modern C++20 features
- Add comprehensive documentation
- Include unit tests for new features

### Testing
```bash
# Build with tests enabled
./build-client.sh -t Debug --enable-tests

# Run tests
cd build && ctest --verbose
```

## License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.

## Support

- **Documentation**: See the main project README
- **Issues**: Report bugs on GitHub Issues
- **Discussions**: Join GitHub Discussions
- **Security**: Report security issues privately

## Roadmap

### Upcoming Features
- Voice and video calling
- Screen sharing
- Group chat rooms
- Mobile companion app
- Plugin marketplace
- Advanced message formatting
- Emoji reactions
- Message threading

### Performance Improvements
- WebAssembly plugins
- GPU-accelerated rendering
- Advanced caching strategies
- Network optimization
- Memory usage reduction