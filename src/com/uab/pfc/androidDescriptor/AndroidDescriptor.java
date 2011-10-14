package com.uab.pfc.androidDescriptor;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class AndroidDescriptor extends Activity {
    /** Called when the activity is first created. */
	NativeDescriptor native_descriptor;
	String file;
	float []res;
	TextView text1;
	
	
	private static final String TAG = "AndroidDescriptor";
	
	private static String DIRECTORIODEFECTO    = "/AndroidDescriptor/";
	private static String DIRECTORIOLOGDEFECTO = "/AndroidDescriptor/output/";
	private static String DIRECTORIOIMGDEFECTO = "/PFC/imgs/";
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

	    // Setup the UI
	    Button button = (Button) findViewById(R.id.button1);
	    text1 = (TextView) findViewById(R.id.text1);
	    
	    button.setOnClickListener(new OnClickListener() {
		    public void onClick(View v) {
		    	startService(new Intent(AndroidDescriptor.this ,
		    					DescriptorService.class));   
		    }
	    });
        
	    // Setup the directories
	    comprobarDirectorios();
    }
    
    
    
    public void comprobarDirectorios() {
    	File directorio = new File(DIRECTORIODEFECTO);
    	AlertDialog alert;
    	AlertDialog.Builder builder;
    	
    	if (!directorio.exists()) {
    		if (directorio.mkdir()) {
    			builder = new AlertDialog.Builder(this);
    	    	builder.setMessage("Creado directorio " + DIRECTORIODEFECTO)
    	    	       .setCancelable(false)
    	    	       .setNeutralButton("Ok", new DialogInterface.OnClickListener() {
    	    	           public void onClick(DialogInterface dialog, int id) {
    	    	        	   dialog.cancel();
    	    	           }
   	    	           });    	    	
    		} else {
    			builder = new AlertDialog.Builder(this);
    	    	builder.setMessage("Error al crear el directorio " +
    	    				DIRECTORIODEFECTO)
    	    	       .setCancelable(false)
		    	       .setNeutralButton("Ok", new DialogInterface.OnClickListener() {
		    	    	   public void onClick(DialogInterface dialog, int id) {
		    	    		   AndroidDescriptor.this.finish();
		    	    	   }
		    	       });       
    		}
    		
    		alert = builder.create();
    	}
    	
    	directorio = new File(DIRECTORIOLOGDEFECTO);
    	if (!directorio.exists()) {
    		if (directorio.mkdir()) {
    			builder = new AlertDialog.Builder(this);
    	    	builder.setMessage("Creado directorio " + DIRECTORIOLOGDEFECTO)
    	    	       .setCancelable(false)
    	    	       .setNeutralButton("Ok", new DialogInterface.OnClickListener() {
    	    	           public void onClick(DialogInterface dialog, int id) {
    	    	        	   dialog.cancel();
    	    	           }
   	    	           });    	    	
    		} else {
    			builder = new AlertDialog.Builder(this);
    	    	builder.setMessage("Error al crear el directorio " +
    	    				DIRECTORIOLOGDEFECTO)
    	    	       .setCancelable(false)
		    	       .setNeutralButton("Ok", new DialogInterface.OnClickListener() {
		    	    	   public void onClick(DialogInterface dialog, int id) {
		    	    		   AndroidDescriptor.this.finish();
		    	    	   }
		    	       });       
    		}
    		
    		alert = builder.create();
    	}
    	
    }
    
    @Override
    protected void onPause() {
        super.onPause();
    }
    
    @Override
    protected void onResume() {
        super.onResume();
    }
}