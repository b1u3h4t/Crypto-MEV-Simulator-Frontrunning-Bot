#!/bin/bash

# MEV Frontrunning Bot - Setup Script
# This script sets up the development environment for the MEV simulator

set -e

echo "🚀 Setting up MEV Frontrunning Bot development environment..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on Ubuntu/Debian
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if command -v apt-get &> /dev/null; then
        PACKAGE_MANAGER="apt"
    elif command -v yum &> /dev/null; then
        PACKAGE_MANAGER="yum"
    else
        print_error "Unsupported Linux distribution. Please install dependencies manually."
        exit 1
    fi
else
    print_error "This script is designed for Linux systems. Please install dependencies manually."
    exit 1
fi

# Update package list
print_status "Updating package list..."
if [[ "$PACKAGE_MANAGER" == "apt" ]]; then
    sudo apt update
elif [[ "$PACKAGE_MANAGER" == "yum" ]]; then
    sudo yum update -y
fi

# Install system dependencies
print_status "Installing system dependencies..."

if [[ "$PACKAGE_MANAGER" == "apt" ]]; then
    sudo apt install -y \
        build-essential \
        cmake \
        git \
        curl \
        wget \
        pkg-config \
        libssl-dev \
        libcurl4-openssl-dev \
        libboost-all-dev \
        libhiredis-dev \
        python3 \
        python3-pip \
        nodejs \
        npm \
        docker.io \
        docker-compose
elif [[ "$PACKAGE_MANAGER" == "yum" ]]; then
    sudo yum groupinstall -y "Development Tools"
    sudo yum install -y \
        cmake \
        git \
        curl \
        wget \
        pkg-config \
        openssl-devel \
        libcurl-devel \
        boost-devel \
        hiredis-devel \
        python3 \
        python3-pip \
        nodejs \
        npm \
        docker \
        docker-compose
fi

# Install Rust (for Foundry/Anvil)
print_status "Installing Rust..."
if ! command -v rustc &> /dev/null; then
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    source ~/.cargo/env
    print_success "Rust installed successfully"
else
    print_status "Rust already installed"
fi

# Install Foundry (for Anvil)
print_status "Installing Foundry..."
if ! command -v anvil &> /dev/null; then
    curl -L https://foundry.paradigm.xyz | bash
    source ~/.bashrc
    foundryup
    print_success "Foundry installed successfully"
else
    print_status "Foundry already installed"
fi

# Install Node.js dependencies
print_status "Installing Node.js dependencies..."
npm install -g yarn
npm install -g hardhat

# Create necessary directories
print_status "Creating project directories..."
mkdir -p logs
mkdir -p data/tx_data
mkdir -p data/results
mkdir -p config
mkdir -p scripts

# Set up environment file
if [[ ! -f .env ]]; then
    print_status "Creating .env file from template..."
    cp env.example .env
    print_warning "Please edit .env file with your configuration"
else
    print_status ".env file already exists"
fi

# Install Python dependencies for analysis
print_status "Installing Python dependencies..."
pip3 install --user \
    pandas \
    numpy \
    matplotlib \
    seaborn \
    jupyter \
    requests \
    web3 \
    eth-account

# Set up Docker (optional)
print_status "Setting up Docker..."
if command -v docker &> /dev/null; then
    sudo systemctl enable docker
    sudo systemctl start docker
    sudo usermod -aG docker $USER
    print_success "Docker configured"
else
    print_warning "Docker not available"
fi

# Build the project
print_status "Building MEV Frontrunning Bot..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# Run tests
print_status "Running tests..."
cd build
make test
cd ..

# Set up Git hooks (optional)
if [[ -d .git ]]; then
    print_status "Setting up Git hooks..."
    mkdir -p .git/hooks
    
    # Pre-commit hook
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
# Pre-commit hook for MEV Frontrunning Bot

echo "Running pre-commit checks..."

# Run clang-format
if command -v clang-format &> /dev/null; then
    echo "Formatting code..."
    find src/ -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
fi

# Run tests
cd build && make test && cd ..

echo "Pre-commit checks passed!"
EOF
    
    chmod +x .git/hooks/pre-commit
    print_success "Git hooks configured"
fi

# Create development scripts
print_status "Creating development scripts..."

# Build script
cat > scripts/build.sh << 'EOF'
#!/bin/bash
set -e

echo "Building MEV Frontrunning Bot..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
echo "Build completed successfully!"
EOF

# Test script
cat > scripts/test.sh << 'EOF'
#!/bin/bash
set -e

echo "Running tests..."
cd build
make test
cd ..
echo "Tests completed successfully!"
EOF

# Run script
cat > scripts/run.sh << 'EOF'
#!/bin/bash

# Default configuration
MODE="realtime"
STRATEGIES="arbitrage,sandwich"
CONFIG=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --mode)
            MODE="$2"
            shift 2
            ;;
        --strategies)
            STRATEGIES="$2"
            shift 2
            ;;
        --config)
            CONFIG="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "Starting MEV Frontrunning Bot..."
echo "Mode: $MODE"
echo "Strategies: $STRATEGIES"

if [[ -n "$CONFIG" ]]; then
    ./build/mev_sim_bot --mode "$MODE" --strategies "$STRATEGIES" --config "$CONFIG"
else
    ./build/mev_sim_bot --mode "$MODE" --strategies "$STRATEGIES"
fi
EOF

# Make scripts executable
chmod +x scripts/build.sh
chmod +x scripts/test.sh
chmod +x scripts/run.sh

print_success "Development scripts created"

# Print completion message
echo ""
print_success "🎉 MEV Frontrunning Bot setup completed successfully!"
echo ""
echo "Next steps:"
echo "1. Edit .env file with your configuration"
echo "2. Run: ./scripts/build.sh"
echo "3. Run: ./scripts/test.sh"
echo "4. Run: ./scripts/run.sh --mode realtime --strategies arbitrage"
echo ""
echo "Documentation: README.md"
echo "Configuration: config/default_config.json"
echo ""

# Check if user needs to log out and back in for Docker
if groups $USER | grep -q docker; then
    print_success "Docker group membership active"
else
    print_warning "Please log out and log back in for Docker group membership to take effect"
fi

print_success "Setup complete! 🚀"
