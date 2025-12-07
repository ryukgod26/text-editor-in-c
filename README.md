
# Kilo Text Editor - A Terminal-Based Text Editor in C
## Project Overview

**Kilo** is a small terminal-based text editor written entirely in C. This project demonstrates the fundamentals of building a text editor from scratch, implementing features like raw mode terminal handling, cursor movement, file I/O, text Highlighting and text manipulation operations. The Text Editor has all the basic Features of a Normal Text Editor and a feature of code editor like syntax Highlighting for c or cpp files.I will try to add Undo/Redo Feature in future.

## Key Features

### Editing Features
*   **Full Text Editing:** Insert, delete, and modify text with complete character support
*   **Multi-line Support:** Handle files with unlimited number of lines
*   **Cursor Navigation:** Arrow key navigation with proper line wrapping
*   **File Operations:** Open, save, and create new files with unsaved changes protection
*   **Status Bar:** Display filename, line count, cursor position, and modification status
*   **Message Bar:** Show temporary status messages and prompts

### Advanced Functionality
*   **Smart Scrolling:** Viewport scrolling for files larger than terminal size
*   **Tab Rendering:** Proper tab character handling with configurable tab stops (8 spaces)
*   **Line Wrapping:** Intelligent cursor movement across line boundaries
*   **Memory Management:** Dynamic memory allocation for efficient file handling
*   **Raw Mode Terminal:** Complete control over terminal input/output
*   **Cross-platform:** Works on Unix/Linux systems with termios support

### Keyboard Shortcuts
*   **Ctrl+Q:** Quit editor (with unsaved changes warning)
*   **Ctrl+S:** Save file (prompts for filename if new file)
*   **Arrow Keys:** Navigate cursor in all directions
*   **Page Up/Down:** Scroll by screen height
*   **Home/End:** Jump to beginning/end of line
*   **Backspace/Delete:** Remove characters
*   **Enter:** Create new line
*   **ESC:** Cancel current operation


#### Key Modules
*   **Terminal Management:** Raw mode setup, cursor positioning, screen clearing
*   **Input Processing:** Keyboard input handling, special key sequences
*   **Display Engine:** Screen rendering, scrolling, status bars
*   **File Operations:** Loading, saving, and managing text files
*   **Text Manipulation:** Insertion, deletion, and modification operations

### 🔧 Technical Features

*   **Raw Terminal Mode:** Bypasses shell processing for direct key handling
*   **ANSI Escape Sequences:** Uses VT100 compatible codes for cursor control
*   **Dynamic Memory Management:** Efficiently handles files of varying sizes
*   **Buffer-based Rendering:** Minimizes screen flicker with buffered output
*   **Cross-platform Compatibility:** Works on any POSIX-compliant system


### Normal Usage

#### Navigation
*   **Arrow Keys:** Move cursor up, down, left, right
*   **Page Up/Down:** Scroll by full screen
*   **Home:** Jump to beginning of current line
*   **End:** Jump to end of current line

#### Editing
*   **Character Press:** Insert characters at cursor position
*   **Enter:** Create new line and move cursor down
*   **Backspace:** Delete character to the left of cursor
*   **Delete:** Delete character at cursor position

#### File Operations
*   **Ctrl+S:** Save current file
    - If new file: prompts for filename
    - Shows "X bytes written to disk" on success
*   **Ctrl+Q:** Quit editor
    - Warns if unsaved changes exist
    - Requires 3 consecutive Ctrl+Q presses to force quit

#### Interface Elements
*   **Status Bar:** Shows filename, line count, modification status, cursor position
*   **Message Bar:** Displays temporary messages and prompts
*   **Welcome Screen:** Shows version info when no file is loaded

### Some Advanced Features

#### Tab Handling
*   Tabs are rendered as spaces (default: 8 spaces)
*   Configurable via `TAB_STOP` constant
*   Proper cursor positioning with tab alignment

#### Scrolling System
*   Automatic scrolling when cursor moves beyond screen boundaries
*   Horizontal and vertical scrolling support
*   Smooth viewport management for large files

#### Memory Management
*   Dynamic allocation for text rows
*   Efficient reallocation when inserting/deleting lines
*   Automatic cleanup on exit
