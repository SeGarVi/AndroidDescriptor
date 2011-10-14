package com.uab.pfc.androidDescriptor;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import android.app.IntentService;
import android.app.Service;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap.Config;
import android.os.Environment;
import android.os.IBinder;
import android.util.Log;

public class DescriptorService extends IntentService{

	public DescriptorService() {
		super("DescriptorService");
		// TODO Auto-generated constructor stub
	}
	NativeDescriptor native_descriptor;
	
	private static final String TAG = "AndroidDescriptor";
	
	private static String DIRECTORIODEFECTO =
			"/AndroidDescriptor/";
	private static String DIRECTORIOLOGDEFECTO =
				"/AndroidDescriptor/output/";
	private static String DIRECTORIOIMGDEFECTO =
				"/PFC/imgs/";
	private int n_ejecuciones = 5;
	

	float []profiling;
	
	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return null;
	}

	
	@Override	
	public void onCreate() {    
	    super.onCreate();  
	    native_descriptor = new NativeDescriptor();
	    Intent intent=new Intent("com.uab.pfc.androidDescriptor.DescriptorService");  
	    this.startService(intent);
	}	
	@Override	
	public void onDestroy() {	

		}	
	@Override
	public void onHandleIntent(Intent arg0) {
		Bitmap bitmapOrig = null;
    	Bitmap bitmap = null;
    	File directorio;
    	int contador = 0;
    	String []archivos;
    	String impr;
    	File root = Environment.getExternalStorageDirectory();
    	int i, j;
    	
    	double tiempos[][]     = new double[n_ejecuciones][14];
    	double medias[]        = new double[14];
    	double desviaciones[] = new double[14];
    	
    	// load bitmap from resources
        BitmapFactory.Options options = new BitmapFactory.Options();
        // Make sure it is 24 bit color as our image processing algorithm expects this format
        options.inPreferredConfig = Config.ARGB_8888;
    	
    	directorio = new File(root.getAbsolutePath() + DIRECTORIOIMGDEFECTO);
    	archivos = directorio.list();
    	
    	for (String archivo : archivos) {
    		if (!archivo.equals(".directory")) {
				contador++;
				Log.i(TAG, "Analizando " +
						archivo 	  + " (" +
						String.valueOf(contador) + "/" +
						String.valueOf(archivos.length) + ")");
				

		    	for (i = 0; i < n_ejecuciones; i++) {
		    		Log.i(TAG, "Iteracion " + String.valueOf(i));
		    		profiling = native_descriptor.descriptor(root.getAbsolutePath() + DIRECTORIOIMGDEFECTO + archivo);
	
		    		for (j = 0; j < 14; j++)
		    			tiempos[i][j] = profiling[j+3];
		    	}
	
		    	for (j = 0; j < 14; j++)
		    		medias[j] = 0;
	
		    	for (j = 0; j<14; j++) {
		    		for (i = 0; i < n_ejecuciones; i++)
		    			medias[j] += tiempos[i][j];
		    		medias[j] /= n_ejecuciones;
		    	}
	
		    	for (j = 0; j<14; j++) {
		    		desviaciones[j] = 0;
		    		for (i = 0; i < n_ejecuciones; i++)
		    			desviaciones[j] += Math.pow(tiempos[i][j] - medias[j], 2);
		    		desviaciones[j] /= n_ejecuciones;
		    		desviaciones[j] = Math.sqrt(desviaciones[j]);
		    	}
		    	/****/
		    	
				
				impr = String.valueOf((int)profiling[0]) + ";" +
					   String.valueOf((int)profiling[1]) + ";"+
					   String.valueOf((int)profiling[2]) + ";";
				
				
				/****/
				for (i = 0; i < 14; i++)
					impr += String.valueOf(medias[i]) + ";";
	
				for (i = 0; i < 13; i++)
					impr += String.valueOf(desviaciones[i]) + ";";
				impr += String.valueOf(desviaciones[13]);
				 /****/
				
				/*for (i = 0; i < profiling.length; i++) {
					impr += String.valueOf(profiling[i]) + ";";
				}*/
				
				Log.i(TAG, "Finalizado " +
						archivo 	  + " (" +
						String.valueOf(contador) + "/" +
						String.valueOf(archivos.length) + ")");
				
				try {
				    if (root.canWrite()){
				        File gpxfile = new File(root.getAbsolutePath() + DIRECTORIOLOGDEFECTO,  archivo + ".prof");
				        FileWriter gpxwriter = new FileWriter(gpxfile);
				        BufferedWriter out = new BufferedWriter(gpxwriter);
				        out.write(impr);
				        out.close();
				    }
				} catch (IOException e) {
				    Log.e(TAG, "Could not write file " + e.getMessage());
				}
    		}
    	}
	}
}
