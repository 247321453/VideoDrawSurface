package com.max.ffmpegnativewindow;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

@SuppressLint("Registered")
public class BaseActivity extends AppCompatActivity {
    @Override
    public final void onCreate(@Nullable Bundle savedInstanceState, @Nullable PersistableBundle persistentState) {
        super.onCreate(savedInstanceState, persistentState);
    }

    @Override
    protected final void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        doOnCreate(savedInstanceState);
        checkRequestPermissions();
    }

    protected void doOnCreate(@Nullable Bundle savedInstanceState) {

    }

    private void checkRequestPermissions() {
        String[] permissions = getRequestPermissions();
        if (permissions != null) {
            List<String> pers = new ArrayList<>();
            for (String permission : permissions) {
                if (PackageManager.PERMISSION_GRANTED != ActivityCompat.checkSelfPermission(this, permission)) {
                    pers.add(permission);
                }
            }
            if (pers.size() > 0) {
                String[] ps = new String[pers.size()];
                ActivityCompat.requestPermissions(this, pers.toArray(ps), 0);
            } else {
                initData();
            }
        } else {
            initData();
        }
    }

    protected String[] getRequestPermissions() {
        return null;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        initData();
    }

    protected void initData() {

    }


    @SuppressWarnings("unchecked")
    protected <T extends View> T bind(int id) {
        return (T) findViewById(id);
    }

    @SuppressWarnings("unchecked")
    protected <T extends View> T $(int id) {
        return bind(id);
    }

    @SuppressWarnings("UnusedReturnValue")
    protected <T extends View> T bindClick(int id, View.OnClickListener clickListener) {
        T t = findViewById(id);
        if (t != null && clickListener != null) {
            t.setOnClickListener(clickListener);
        }
        return t;
    }

    public Toolbar enableBackHome(@IdRes int id) {
        Toolbar toolbar = setupToolbar(id);
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
        return toolbar;
    }


    protected void hideToolBar(@IdRes int id) {
        Toolbar toolbar = bind(id);
        if (toolbar != null) {
            toolbar.setVisibility(View.GONE);
        }
    }

    protected <T extends CheckBox> T bindCheck(int id, CompoundButton.OnCheckedChangeListener clickListener) {
        T t = findViewById(id);
        if (t != null && clickListener != null) {
            t.setOnCheckedChangeListener(clickListener);
        }
        return t;
    }

    public Context getContext() {
        return this;
    }

    //fragment
    public Activity getActivity() {
        return this;
    }

    public BaseActivity getCompatActivity() {
        return this;
    }

    public Toolbar setupToolbar(@IdRes int id) {
        Toolbar toolbar = bind(id);
        if (toolbar != null) {
            setSupportActionBar(toolbar);
        }
        return toolbar;
    }

    public void setActivityTitle(int title) {
        setActivityTitle(getString(title));
    }

    public void setActivityTitle(String title) {
        setTitle(title);
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setTitle(title);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private Toast toast;

    @SuppressLint("ShowToast")
    public void showToastMessage(String msg, int type) {
        try {
            if (toast == null) {
                toast = Toast.makeText(this.getApplicationContext(), msg, type);
            } else {
                toast.setText(msg);
            }
            toast.show();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void showToastMessage(int msg) {
        showToastMessage(getString(msg));
    }

    @SuppressLint("ShowToast")
    public void showToastMessage(String msg) {
        try {
            if (toast == null) {
                toast = Toast.makeText(this, msg, Toast.LENGTH_SHORT);
            } else {
                toast.setText(msg);
            }
            toast.show();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
