# .vs-remover

🧹 .VS  Cleaner

A lightweight C++ utility for cleaning up .vs (Visual Studio) directories recursively from a given root path, while allowing exclusions via a text file.

Perfect for freeing up disk space and removing unnecessary IDE cache directories.

💡 Features
Recursively finds and removes .vs folders.

Supports exclusion paths to skip specific projects or directories.

Provides a test run mode to preview which folders would be deleted.

🚀 Usage
Once compiled and run, you’ll see a menu:

.VS Directory Cleaner
--------------------
1) Clean .vs directories
2) Add exclusion path
3) View exclusions
4) Test run (show what would be deleted)
5) Exit


1️⃣ Clean .vs directories
Deletes all found .vs folders from the root search path (C:\ by default), skipping excluded ones.

2️⃣ Add exclusion path
Appends a new exclusion to excluded.txt.
Example: C:\\\Users\duh\Desktop\usermode ( it has to be C:\\ instead of C:\ )

3️⃣ View exclusions
Displays the currently loaded exclusion list.

4️⃣ Test run
Shows which .vs folders would be deleted without deleting them.




🧼 Sample Output

WILL BE DELETED: C:\\\\Projects\MyApp\\.vs (450 MB)

EXCLUDED (will NOT be deleted): C:\\\\Users\duh\Desktop\usermode\\.vs (120 MB)

Deleted 1 .vs directories (450 MB)

Preserved 1 excluded directories (120 MB)



🧾 License
This project is released under the MIT License.
Feel free to modify and use it in personal or commercial projects.

