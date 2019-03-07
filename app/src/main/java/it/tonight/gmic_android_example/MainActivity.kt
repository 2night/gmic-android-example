package it.tonight.gmic_android_example

import android.app.ProgressDialog
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.util.Log
import android.widget.Button
import android.widget.ImageView
import org.jetbrains.anko.doAsync
import java.io.BufferedOutputStream
import java.io.BufferedReader
import java.io.FileOutputStream
import java.io.InputStreamReader




class MainActivity : AppCompatActivity() {

    fun gmic(input : Bitmap, command : String) : Bitmap
    {
        // I put update250.gmic on cache dir to make it available to executable
        Log.d("GMIC", "Loading stdlib...")
        val tmpPath = cacheDir.resolve("gmic/update250.gmic")
        if( !tmpPath.exists() ) {
            cacheDir.resolve("gmic").mkdirs()
            assets.open("gmic/update250.gmic").use { inStream ->
                tmpPath.outputStream().use{ outStream ->
                    inStream.copyTo(outStream)
                }
            }
        }

        // Gmic is not compiled with jpg, png or other library
        // It can only read ppm, bmp.
        val inputFile = cacheDir.resolve("tmp_input.ppm")
        val outputFile = cacheDir.resolve("tmp_output.bmp")

        // Remove spurious files
        if (inputFile.exists()) inputFile.delete()
        if (outputFile.exists()) outputFile.delete()

        // Create a ppm from bitmap.
        Log.d("GMIC", "Creating input file...")
        val arr = IntArray(input.width*input.height)
        input.getPixels(arr,0,input.width,0,0,input.width,input.height);

        val out = BufferedOutputStream(FileOutputStream(inputFile))
        out.write(("P6 \n" + input.width.toString() +  " " + input.height.toString() + " \n255 \n").toByteArray())
        for(i in 0 until arr.size)
        {
            val r = ((arr[i] ushr 16) and 0xff).toByte()
            val g = ((arr[i] ushr 8) and 0xff).toByte()
            val b = ((arr[i] ushr 0) and 0xff).toByte()

            out.write(byteArrayOf(r,g,b))
        }

        out.flush()
        out.close()

        // Command to send
        Log.d("GMIC", "Apply Effect...")
        val path = applicationContext.getApplicationInfo().nativeLibraryDir + "/gmic.so " + inputFile.absoluteFile + " " + command +" -output " + outputFile.absoluteFile

        // Exec g'mic and capture stderr to log errors and status
        val process = Runtime.getRuntime().exec(path, arrayOf("GMIC_PATH=" + applicationContext.cacheDir))
        val reader = BufferedReader(InputStreamReader(process.errorStream))

        val buffer = CharArray(4096)
        val output = StringBuffer()

        while (true) {
            val read = reader.read(buffer)
            if (read <= 0) break
            output.append(buffer, 0, read);
        }

        reader.close()
        process.waitFor()
        println(output)

        // Read bitmap from output file
        Log.d("GMIC", "Done")
        val result = BitmapFactory.decodeFile(outputFile.absolutePath)

        // Delete temporary files
        if (inputFile.exists()) inputFile.delete()
        if (outputFile.exists()) outputFile.delete()

        return result
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val image = findViewById<ImageView>(R.id.image)
        val button = findViewById<Button>(R.id.button)

        button.setOnClickListener {

            // Show a progress dialog
            val progressDialog = ProgressDialog(this)
            progressDialog.isIndeterminate = true
            progressDialog.setMessage("Applying effect...")
            progressDialog.setCancelable(false)
            progressDialog.show()

            // Disable button during processing
            button.isEnabled = false


            doAsync {
                val exampleBitmap = BitmapFactory.decodeResource(resources, R.drawable.example)

                // Apply bokeh effect!
                val outputBitmap = gmic(
                    exampleBitmap,
                    "fx_bokeh 3,8,0,30,8,4,0.3,0.2,210,210,80,160,0.7,30,20,20,1,2,170,130,20,110,0.15,0"
                )

                // Finished. Hide progress dialog.
                runOnUiThread {
                    progressDialog.dismiss()
                    image.setImageBitmap(outputBitmap)
                    exampleBitmap.recycle()
                    button.isEnabled = true
                }

            }
        }
    }

}
