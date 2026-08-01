// GameDataGuardWindow.cs
//
// Custom Unity Editor window for running GameDataGuard validation.
//
// Open via: Tools > GameDataGuard
//
// This window does not require Play Mode.  Validation calls the native DLL
// directly through the P/Invoke wrapper in GameDataGuardNative.cs.

using UnityEditor;
using UnityEngine;

namespace GameDataGuard.Editor
{
    public class GameDataGuardWindow : EditorWindow
    {
        // ----------------------------------------------------------------
        // State
        // ----------------------------------------------------------------
        private string           _dataDirectory = "";
        private ValidationResult _result;
        private bool             _hasValidated;
        private Vector2          _scrollPos;

        // ----------------------------------------------------------------
        // Menu entry
        // ----------------------------------------------------------------

        [MenuItem("Tools/GameDataGuard")]
        public static void ShowWindow()
        {
            var window = GetWindow<GameDataGuardWindow>("GameDataGuard");
            window.minSize = new Vector2(540f, 420f);
            window.Show();
        }

        // ----------------------------------------------------------------
        // GUI
        // ----------------------------------------------------------------

        private void OnGUI()
        {
            DrawDirectoryRow();
            DrawActionRow();
            GUILayout.Space(8f);
            DrawResults();
        }

        private void DrawDirectoryRow()
        {
            GUILayout.Label("Game Data Directory", EditorStyles.boldLabel);
            GUILayout.BeginHorizontal();
            _dataDirectory = EditorGUILayout.TextField(_dataDirectory);
            if (GUILayout.Button("Select Directory", GUILayout.Width(130f)))
            {
                string chosen = EditorUtility.OpenFolderPanel(
                    "Select Game Data Directory",
                    _dataDirectory,
                    "");
                if (!string.IsNullOrEmpty(chosen))
                    _dataDirectory = chosen;
            }
            GUILayout.EndHorizontal();
        }

        private void DrawActionRow()
        {
            GUILayout.Space(4f);
            bool hasPath = !string.IsNullOrWhiteSpace(_dataDirectory);

            GUILayout.BeginHorizontal();

            using (new EditorGUI.DisabledScope(!hasPath))
            {
                if (GUILayout.Button("Validate"))
                    RunValidation();
            }

            if (GUILayout.Button("Clear"))
                ClearResults();

            GUILayout.EndHorizontal();
        }

        private void DrawResults()
        {
            if (!_hasValidated)
            {
                EditorGUILayout.HelpBox(
                    "Select a game-data directory and click Validate.",
                    MessageType.None);
                return;
            }

            if (_result == null)
            {
                EditorGUILayout.HelpBox(
                    "No result available.", MessageType.Warning);
                return;
            }

            DrawSummaryBanner();

            if (!_result.IsToolError)
            {
                GUILayout.Space(6f);
                DrawDiagnosticList();
            }
        }

        private void DrawSummaryBanner()
        {
            if (_result.IsToolError)
            {
                EditorGUILayout.HelpBox(
                    "Tool Error: " + _result.message,
                    MessageType.Error);
                return;
            }

            string statusLabel = _result.IsPassed ? "PASSED" : "FAILED";
            MessageType msgType = _result.IsPassed
                ? MessageType.Info
                : MessageType.Error;

            EditorGUILayout.HelpBox(
                string.Format(
                    "Status: {0}     Errors: {1}     Warnings: {2}",
                    statusLabel,
                    _result.errorCount,
                    _result.warningCount),
                msgType);
        }

        private void DrawDiagnosticList()
        {
            if (_result.diagnostics == null || _result.diagnostics.Length == 0)
            {
                GUILayout.Label("No diagnostics.", EditorStyles.miniLabel);
                return;
            }

            GUILayout.Label("Diagnostics", EditorStyles.boldLabel);

            _scrollPos = EditorGUILayout.BeginScrollView(_scrollPos);

            foreach (var d in _result.diagnostics)
            {
                MessageType msgType;
                switch (d.Severity)
                {
                    case DiagnosticSeverity.Error:
                        msgType = MessageType.Error;
                        break;
                    case DiagnosticSeverity.Warning:
                        msgType = MessageType.Warning;
                        break;
                    default:
                        msgType = MessageType.Info;
                        break;
                }

                string location = string.IsNullOrEmpty(d.path)
                    ? d.file
                    : d.file + d.path;

                string entry = string.Format(
                    "[{0}] {1}  {2}\n{3}",
                    d.severity.ToUpper(),
                    d.code,
                    location,
                    d.message);

                EditorGUILayout.HelpBox(entry, msgType);
            }

            EditorGUILayout.EndScrollView();
        }

        // ----------------------------------------------------------------
        // Actions
        // ----------------------------------------------------------------

        private void RunValidation()
        {
            _result       = GameDataGuardNative.ValidateDirectory(_dataDirectory);
            _hasValidated = true;
            Repaint();
        }

        private void ClearResults()
        {
            _result       = null;
            _hasValidated = false;
            Repaint();
        }
    }
}
