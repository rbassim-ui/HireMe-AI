# HireMe AI Architecture

## Overview
HireMe AI is an AI-powered interview simulator designed to help candidates practice and improve their interview skills through realistic simulations with AI-powered feedback.

## System Architecture

### Backend (C)
The backend is written in C and handles:
- **Database Management**: SQLite database for storing users, interviews, questions, and results
- **Interview Engine**: Manages interview flow and question delivery
- **API Communication**: Integrates with external AI services for response evaluation
- **Report Generation**: Creates comprehensive performance reports

### Frontend (Web)
The frontend is built with HTML5, CSS3, and JavaScript:
- **index.html**: Home page with navigation
- **interview.html**: Interview interface for answering questions
- **result.html**: Results page with performance analytics
- **style.css**: Responsive design and styling
- **script.js**: Client-side logic and API communication

## File Structure

```
HireMe-AI/
├── backend-c/          # C backend source code
│   ├── main.c         # Application entry point
│   ├── menu.c         # User interface menus
│   ├── interview.c    # Interview engine
│   ├── api.c          # External API communication
│   ├── db.c           # Database operations
│   ├── json.c         # JSON parsing
│   ├── report.c       # Report generation
│   └── utils.c        # Utility functions
├── frontend/          # Web frontend
│   ├── index.html     # Home page
│   ├── interview.html # Interview page
│   ├── result.html    # Results page
│   ├── style.css      # Stylesheets
│   └── script.js      # Frontend logic
├── tests/             # Unit tests
│   ├── test_db.c      # Database tests
│   ├── test_api.c     # API tests
│   └── test_score.c   # Scoring tests
├── docs/              # Documentation
│   ├── architecture.md # This file
│   └── installation.md # Installation guide
├── Makefile           # Build configuration
└── README.md          # Project overview
```

## Key Components

### Database (db.c)
- Manages SQLite database connections
- Stores user profiles, interview history, and results
- Handles CRUD operations for all data models

### Interview Engine (interview.c)
- Orchestrates the interview flow
- Delivers questions from database
- Records user responses and timing

### API Integration (api.c)
- Sends user responses to AI evaluation service
- Receives feedback and scoring data
- Handles error recovery and retries

### Report Generation (report.c)
- Analyzes interview performance
- Calculates metrics and scores
- Generates detailed feedback reports

## Build System
The project uses GNU Make with GCC compiler and SQLite3 library.

```bash
make         # Build the application
make clean   # Remove build artifacts
make test    # Run unit tests
```

## Technology Stack
- **Backend**: C, SQLite3
- **Frontend**: HTML5, CSS3, JavaScript (Vanilla)
- **Build Tool**: GNU Make
- **Testing**: C Unit Tests
- **External APIs**: AI Evaluation Service (REST API)

## Future Enhancements
- WebSocket support for real-time feedback
- Multiple language support
- Machine learning for personalized recommendations
- Video recording capability
- Mobile application
