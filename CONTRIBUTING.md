# Contributing to SecureChat

Thank you for your interest in contributing to SecureChat! This document provides guidelines and information for contributors.

## 🚀 Getting Started

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- Qt 6.2+ (for GUI client)
- OpenSSL 3.0+
- Docker & Docker Compose

### Development Setup
1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/securechat.git
   cd securechat
   ```
3. Install dependencies:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential cmake libssl-dev libpq-dev qt6-base-dev
   
   # macOS
   brew install cmake openssl qt@6
   ```
4. Build the project:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

## 📝 How to Contribute

### Reporting Issues
- Use GitHub Issues to report bugs
- Include system information, steps to reproduce, and expected behavior
- For security issues, please email security@securechat.org instead

### Suggesting Features
- Open a GitHub Issue with the "enhancement" label
- Describe the feature, use case, and potential implementation
- Discuss the feature before starting implementation

### Code Contributions

#### 1. Create a Feature Branch
```bash
git checkout -b feature/your-feature-name
# or
git checkout -b bugfix/issue-description
```

#### 2. Make Your Changes
- Follow the coding standards (see below)
- Add tests for new functionality
- Update documentation as needed
- Ensure all tests pass

#### 3. Commit Your Changes
```bash
git add .
git commit -m "feat: add new feature description

- Detailed description of changes
- Any breaking changes
- Closes #issue-number"
```

#### 4. Push and Create Pull Request
```bash
git push origin feature/your-feature-name
```
Then create a Pull Request on GitHub.

## 🎯 Coding Standards

### C++ Guidelines
- Follow modern C++20 practices
- Use RAII for resource management
- Prefer smart pointers over raw pointers
- Use const correctness
- Follow the Google C++ Style Guide with these exceptions:
  - Use 4 spaces for indentation
  - Line length limit: 100 characters

### Code Formatting
```bash
# Format code with clang-format
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Naming Conventions
- **Classes**: PascalCase (`MessageHandler`)
- **Functions/Methods**: camelCase (`sendMessage`)
- **Variables**: snake_case (`message_count`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_CONNECTIONS`)
- **Namespaces**: lowercase (`securechat::network`)

### Documentation
- Use Doxygen comments for public APIs
- Include examples in documentation
- Update README.md for significant changes

## 🧪 Testing

### Running Tests
```bash
# Build with tests
cmake -DENABLE_TESTS=ON ..
make -j$(nproc)

# Run all tests
ctest --verbose

# Run specific test
./bin/test_encryption
```

### Writing Tests
- Use Google Test framework
- Test both positive and negative cases
- Include performance tests for critical paths
- Mock external dependencies

### Test Coverage
```bash
# Generate coverage report
cmake -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
make test
make coverage
```

## 🔒 Security Guidelines

### Security Best Practices
- Never commit secrets, keys, or passwords
- Use secure coding practices
- Validate all inputs
- Follow OWASP guidelines
- Report security issues privately

### Cryptographic Code
- Use established libraries (OpenSSL)
- Don't implement custom crypto
- Follow current best practices
- Include security tests

## 📋 Pull Request Guidelines

### Before Submitting
- [ ] Code follows style guidelines
- [ ] Tests pass locally
- [ ] Documentation updated
- [ ] No merge conflicts
- [ ] Commit messages are clear

### PR Description Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests pass
- [ ] Manual testing completed

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No breaking changes (or documented)
```

## 🏷️ Commit Message Format

Use conventional commits:
```
type(scope): description

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting
- `refactor`: Code restructuring
- `test`: Adding tests
- `chore`: Maintenance

Examples:
```
feat(encryption): add AES-256 support
fix(network): resolve connection timeout issue
docs(readme): update build instructions
```

## 🌟 Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes
- Project documentation

## 📞 Getting Help

- GitHub Discussions for questions
- Discord server: [invite-link]
- Email: dev@securechat.org

## 📄 License

By contributing, you agree that your contributions will be licensed under the MIT License.

Thank you for contributing to SecureChat! 🚀