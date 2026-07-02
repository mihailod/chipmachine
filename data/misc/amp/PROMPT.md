# Instrunctions

Write the python code with the following considerations:

* The program purpose is to scrape the AMP web site
* Should use the PROGRESS.csv file with one column `POSITION` that is holding the value of the next position to use in the HTTP request
  * Append the position value that is to be used next
  * Before starting the page requests, first read the last position and start the requests with that position
  * on successful request increase position for 50 and append to the PROGRESS.csv file and continue with the requests
* During one program run keep counter of HTTP request.s Exit after 1000 requests
* Between two HTTP requests ensure a 2s delay to avoid distrurbing site operations
* Set the appropriate HTTP headers to avoid being blocked by bot blockers
* Use this URL to form the HTTP request: `https://amp.dascene.net/newresult.php?request=module&search=&position=<POSITION>`
  * Replace the placeholder `<POSITION>` with the current position as tracked in the PROGRESS.csv file
* HTTP request result will have the HTML table with the following table row:
```
<tr><th>Module</th><th>Composer</th><th>Format</th><th>Size</th><th>DL</th><th>Infos</th></tr>
```
* Example of table row data:
```
<tr class="tr0"><td><a href="downmod.php?index=58512&amp;application=AMP" target="_new">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;eyeZ&nbsp;-</a></td><td><a href="detail.php?view=6104">Ramone</a></td><td>XM</td><td>636Kb</td><td>215</td><td><a href="analyzer2.php?idx=58512" target="_modinfo"><img src="images/info.png"></a></td></tr>
```
* Program should parse the table row data and extract the folowing columns to append to the resulting file: MODULES.csv
  * `MODULE_ID` - in the first table row with header "Module" will be HTML link with "href" attribute. Example: `downmod.php?index=58512&amp;application=AMP`; Column value should be the "index" parameter
  * `MODULE_TITLE` - from the HTML link mentined above extract the link text as the value for this column. Example: `&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;eyeZ&nbsp;-`. Ensure that all leading and trailing blank spaces (&nbsp) are trimed or replaced with URI decoded value otherwise: Example result value: `- eyeZ -`
  * `COMPOSER` - value from the next table data (with a header `Composer`). It will be the value of the link text of the HTML link
  * `COMPOSER_DETAIL_URL` - value from the "href" attribute of the HTML link
  * `FORMAT` - next value (with a header `Format`)
  * `SIZE` - next value
  * `DL` - next value
