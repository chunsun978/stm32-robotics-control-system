# CI/CD Documentation

## Overview

This project uses GitHub Actions for continuous integration and deployment. Every push and pull request triggers automated builds, tests, and quality checks.

## Workflows

### 1. Build Workflow (`build.yml`)

**Triggers**: Push to `main` or `dev`, Pull Requests  
**Purpose**: Compile firmware for Debug and Release configurations

**What it does**:
- ✅ Installs ARM GCC toolchain
- ✅ Configures CMake with presets
- ✅ Builds Debug and Release versions
- ✅ Reports memory usage (Flash/RAM)
- ✅ Uploads firmware artifacts (.elf, .bin, .hex)

**Artifacts**: Available for 30 days after build

---

### 2. Code Quality Workflow (`code-quality.yml`)

**Triggers**: Push to `main` or `dev`, Pull Requests  
**Purpose**: Static analysis and code quality checks

**What it does**:
- ✅ **cppcheck** - Static code analysis
- ✅ **clang-format** - Code formatting verification
- ✅ **lizard** - Complexity analysis
- ✅ **Line counting** - Track codebase size

**Reports Generated**:
- cppcheck XML report
- Complexity metrics
- Lines of code statistics

---

### 3. Release Workflow (`release.yml`)

**Triggers**: Git tags matching `v*.*.*` (e.g., `v1.0.0`)  
**Purpose**: Automated release creation with firmware binaries

**What it does**:
- ✅ Builds optimized Release firmware
- ✅ Generates release notes with memory usage
- ✅ Creates SHA256 checksums
- ✅ Publishes GitHub Release with artifacts
- ✅ Includes flashing instructions

---

## Status Badges

Add these to your main README.md:

```markdown
[![STM32 Build](https://github.com/chunsun978/stm32-robotics-control-system/workflows/STM32%20Build/badge.svg)](https://github.com/chunsun978/stm32-robotics-control-system/actions)
[![Code Quality](https://github.com/chunsun978/stm32-robotics-control-system/workflows/Code%20Quality/badge.svg)](https://github.com/chunsun978/stm32-robotics-control-system/actions)
```

---

## Using CI/CD

### For Developers

#### Every Commit
1. Push to `dev` branch
2. CI automatically builds and tests
3. Check Actions tab for results
4. Review artifacts and reports

#### Pull Requests
1. Create PR from `dev` to `main`
2. CI runs all checks automatically
3. Review build reports in PR
4. Merge when all checks pass

#### Creating a Release
1. Update version in code (if applicable)
2. Commit and push to `main`
3. Create and push a tag:
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```
4. CI creates GitHub Release automatically
5. Download firmware from Releases page

---

## Artifacts

### Build Artifacts
Available after every successful build:

- `stm32-robotics-control.elf` - ELF file with debug symbols
- `stm32-robotics-control.bin` - Raw binary for flashing
- `stm32-robotics-control.hex` - Intel HEX format

**Download**: Actions tab → Select workflow run → Artifacts section

### Reports
- **cppcheck-report.xml** - Static analysis results
- **lizard-report.txt** - Complexity metrics
- **checksums.txt** - SHA256 checksums (releases only)

---

## Memory Usage Tracking

Every build reports:
- Flash usage (text + data)
- RAM usage (data + bss)
- Percentage of available memory

**View**: Check the workflow run summary for detailed reports

---

## Local Testing

Test CI locally before pushing:

### Build Test
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install gcc-arm-none-eabi cmake ninja-build

# Build Debug
cmake --preset Debug
cmake --build build/Debug

# Build Release
cmake --preset Release
cmake --build build/Release
```

### Code Quality Test
```bash
# Install tools
sudo apt-get install cppcheck clang-format
pip install lizard

# Run cppcheck
cppcheck --enable=all --std=c++17 Core/Src/modules/ Core/Inc/modules/

# Check formatting
clang-format --dry-run Core/Src/modules/**/*.cpp

# Run complexity analysis
lizard -l cpp Core/Src/modules/
```

---

## Troubleshooting

### Build Fails
1. Check error logs in Actions tab
2. Verify CMakePresets.json is committed
3. Ensure all source files are tracked in git
4. Test build locally first

### Artifacts Not Generated
1. Check if build succeeded
2. Verify artifact upload step didn't fail
3. Artifacts expire after 30 days

### Release Not Created
1. Verify tag format: `v1.0.0` (must start with 'v')
2. Check if tag was pushed: `git push origin v1.0.0`
3. Review release workflow logs

---

## Best Practices

### ✅ Do
- Push to `dev` branch for development
- Create PRs to merge into `main`
- Test locally before pushing
- Use semantic versioning for releases (v1.0.0)
- Review CI reports before merging

### ❌ Don't
- Push directly to `main` (use PRs)
- Ignore CI failures
- Merge with failing checks
- Create releases without testing

---

## Workflow Customization

### Modify Build Matrix
Edit `.github/workflows/build.yml`:
```yaml
strategy:
  matrix:
    build_type: [Debug, Release, MinSizeRel]  # Add more configs
```

### Add More Checks
Create new workflow in `.github/workflows/`:
```yaml
name: My Custom Check
on: [push, pull_request]
jobs:
  my-job:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      # Your steps here
```

### Change Retention
Modify artifact retention days:
```yaml
- uses: actions/upload-artifact@v4
  with:
    retention-days: 90  # Change from 30
```

---

## Performance

- **Build time**: ~2-3 minutes per configuration
- **Code quality**: ~1-2 minutes
- **Total CI time**: ~5-8 minutes
- **Concurrent jobs**: Build and quality checks run in parallel

---

## GitHub Actions Limits

**Free tier** (public repos):
- ✅ Unlimited minutes
- ✅ Unlimited artifact storage (with retention limits)
- ✅ Concurrent jobs: 20

**Private repos**:
- 2000 minutes/month
- 500 MB artifact storage

---

## Future Enhancements

- [ ] Unit testing integration
- [ ] Code coverage reporting
- [ ] Dependency scanning
- [ ] Security vulnerability scanning
- [ ] Auto-generated documentation
- [ ] Hardware-in-the-loop testing
- [ ] Performance benchmarks

---

## Resources

- [GitHub Actions Docs](https://docs.github.com/en/actions)
- [ARM GCC Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain)
- [CMake Documentation](https://cmake.org/documentation/)
- [cppcheck Manual](http://cppcheck.sourceforge.net/manual.pdf)

