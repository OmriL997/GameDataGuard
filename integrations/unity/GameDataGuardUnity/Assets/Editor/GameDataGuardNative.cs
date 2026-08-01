// GameDataGuardNative.cs
//
// P/Invoke declarations for gamedataguard_unity.dll.
//
// All native interop is isolated in this class.
// GameDataGuardWindow never calls DllImport methods directly.
//
// Memory contract
// ---------------
// gdg_validate_directory_utf8 allocates a UTF-8 JSON string on the native heap.
// The caller must pass the returned pointer to gdg_free_string to release it.
// This is always done in a finally block to prevent leaks regardless of exceptions
// or early returns.
//
// UTF-8 marshaling
// ----------------
// The directory path is encoded as UTF-8 bytes to avoid relying on
// the Windows system code page in the supported Unity Editor environment.
//
// DLL placement
// -------------
// The DLL must be at:
//   Assets/Plugins/x86_64/gamedataguard_unity.dll
// with Plugin Inspector settings:
//   Any Platform: unchecked
//   Editor:       checked
//   Windows x86_64: checked

using System;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;

namespace GameDataGuard.Editor
{
    internal static class GameDataGuardNative
    {
        // DLL name without the .dll extension.
        // Unity resolves this to Assets/Plugins/x86_64/gamedataguard_unity.dll
        // when running in the Editor on Windows x86-64.
        private const string DllName = "gamedataguard_unity";

        // ----------------------------------------------------------------
        // Raw P/Invoke declarations
        // The directory path is passed as a UTF-8 byte array (byte[]).
        // Using byte[] is the most portable way to pass UTF-8 strings across
        // Unity scripting backends without relying on CharSet.Ansi (which
        // would use the system code page rather than UTF-8).
        // ----------------------------------------------------------------

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int gdg_validate_directory_utf8(
            [In] byte[] directoryPathUtf8,
            out IntPtr  resultJsonUtf8);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void gdg_free_string(IntPtr value);

        // ----------------------------------------------------------------
        // Public API
        // ----------------------------------------------------------------

        /// <summary>
        /// Validates a game-data directory and returns a managed ValidationResult.
        /// </summary>
        /// <param name="directoryPath">
        /// Absolute path to the directory containing manifest.json, npcs.json,
        /// items.json, quests.json, and the localization/ subfolder.
        /// </param>
        /// <returns>
        /// A ValidationResult on success or partial validation failure.
        /// Returns a tool-error result when the DLL cannot be loaded, an entry point
        /// cannot be found, the image format is wrong, or the returned JSON is malformed.
        /// </returns>
        internal static ValidationResult ValidateDirectory(string directoryPath)
        {
            IntPtr nativePtr = IntPtr.Zero;

            try
            {
                // Encode the path as NUL-terminated UTF-8 bytes.
                byte[] utf8Path = Encoding.UTF8.GetBytes(directoryPath + "\0");

                int statusCode = gdg_validate_directory_utf8(utf8Path, out nativePtr);

                if (nativePtr == IntPtr.Zero)
                {
                    return MakeToolError(
                        "Native call returned a null result pointer. " +
                        "This is a bug in the native layer.");
                }

                string jsonStr = ReadUtf8String(nativePtr);

                ValidationResult result;
                try
                {
                    result = JsonUtility.FromJson<ValidationResult>(jsonStr);
                }
                catch (Exception ex)
                {
                    return MakeToolError(
                        "Failed to parse native result JSON: " + ex.Message);
                }

                if (result == null)
                {
                    return MakeToolError(
                        "Native result JSON deserialised to null.");
                }

                return result;
            }
            catch (DllNotFoundException ex)
            {
                return MakeToolError(
                    "gamedataguard_unity.dll not found. " +
                    "Copy it to Assets/Plugins/x86_64/ using copy_plugin.ps1 " +
                    "and configure the Plugin Inspector. (" + ex.Message + ")");
            }
            catch (EntryPointNotFoundException ex)
            {
                return MakeToolError(
                    "Native entry point not found — DLL version mismatch? " +
                    "(" + ex.Message + ")");
            }
            catch (BadImageFormatException ex)
            {
                return MakeToolError(
                    "DLL architecture mismatch. " +
                    "Ensure the plugin was built for Windows x86-64. " +
                    "(" + ex.Message + ")");
            }
            catch (Exception ex)
            {
                return MakeToolError("Unexpected interop error: " + ex.Message);
            }
            finally
            {
                // Release the native buffer regardless of how the try block exits.
                if (nativePtr != IntPtr.Zero)
                    gdg_free_string(nativePtr);
            }
        }

        // ----------------------------------------------------------------
        // Helpers
        // ----------------------------------------------------------------

        // Reads a NUL-terminated UTF-8 string from a native pointer.
        // Does not free the pointer; that is the caller's responsibility.
        private static string ReadUtf8String(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return string.Empty;

            // Walk forward until the NUL terminator to find byte length.
            int length = 0;
            while (Marshal.ReadByte(ptr, length) != 0)
                length++;

            if (length == 0)
                return string.Empty;

            byte[] bytes = new byte[length];
            Marshal.Copy(ptr, bytes, 0, length);
            return Encoding.UTF8.GetString(bytes);
        }

        // Constructs a ValidationResult that represents a tool-level failure.
        private static ValidationResult MakeToolError(string message)
        {
            return new ValidationResult
            {
                status       = "tool_error",
                errorCount   = 0,
                warningCount = 0,
                message      = message,
                diagnostics  = Array.Empty<ValidationDiagnostic>()
            };
        }
    }
}
