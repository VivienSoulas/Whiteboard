#!/usr/bin/env php
<?php
// Webserv PHP CGI Test Script
header('Content-Type: text/html');
echo "<!DOCTYPE html>";
echo "<html><head><title>PHP CGI Result</title>";
echo "<style>";
echo "body { font-family: sans-serif; padding: 20px; text-align: center; }";
echo ".card { display: inline-block; background-color: #f1f2f6; padding: 30px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); text-align: left; }";
echo "h1 { color: #2e86de; border-bottom: 2px solid #2e86de; padding-bottom: 10px; }";
echo "ul { list-style-type: none; padding-left: 0; }";
echo "li { padding: 8px 0; border-bottom: 1px dashed #ccc; }";
echo "strong { color: #222f3e; display: inline-block; width: 150px; }";
echo "</style></head><body>";
echo "<div class='card'>";
echo "<h1>🚀 PHP CGI Successfully Executed!</h1>";
echo "<ul>";
echo "<li><strong>Server Name:</strong> " . (isset($_SERVER['SERVER_NAME']) ? $_SERVER['SERVER_NAME'] : 'unknown') . "</li>";
echo "<li><strong>Method:</strong> " . (isset($_SERVER['REQUEST_METHOD']) ? $_SERVER['REQUEST_METHOD'] : 'unknown') . "</li>";
echo "<li><strong>Request URI:</strong> " . (isset($_SERVER['SCRIPT_NAME']) ? $_SERVER['SCRIPT_NAME'] : 'unknown') . "</li>";
echo "<li><strong>Query String:</strong> " . (isset($_SERVER['QUERY_STRING']) && !empty($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '<em>none</em>') . "</li>";
echo "<li><strong>PHP Version:</strong> " . phpversion() . "</li>";
echo "</ul>";

echo "<h3>Environment Dump:</h3>";
echo "<ul>";
foreach ($_SERVER as $key => $value) {
    if (is_scalar($value)) {
        echo "<li><strong>" . htmlspecialchars($key) . ":</strong> " . htmlspecialchars((string)$value) . "</li>";
    }
}
echo "</ul>";

echo "<br><br><a href='/' style='text-decoration: none; background: #2e86de; color: white; padding: 10px 20px; border-radius: 5px; font-weight: bold;'>« Back to Home</a>";
echo "</div>";
echo "</body></html>";
?>
