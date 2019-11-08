from shutil import copyfile
from subprocess import call
import os
import errno

# nuget_installed = 'D:\\ReactNative.Hermes.Windows_nuget\\ReactNative.Hermes.Windows\\installed\\'
nuget_installed = 'D:\\tmp\\v8-nuget\\'

v8_source = 'E:\\JS\\v8\\v8\\'
v8_build_relative = 'out'

platform_x86 = 'Win32'
platform_x64 = 'x64'

flavor_release = 'Release'
flavor_debug = 'Debug'

v8jsi_source = 'D:\\github\\v8jsiruntime\\v8jsi\\'
v8jsi_bin_x64 = v8jsi_source + 'x64\\'
v8jsi_bin_x86 = v8jsi_source

def do_copy(src, dest):
    print('copying ' + src + ' to ' + dest)
    
    try:
        os.makedirs(os.path.dirname(dest))
    except OSError as exc: # Guard against race condition
        if exc.errno != errno.EEXIST:
            raise
    
    copyfile(src, dest)

def copy_v8jsi_bin(triplet, bin_path, bin_file_name):

    if bin_file_name.endswith('.lib'):
        dest_path_segment = '\\lib\\'
    else:
        dest_path_segment = '\\bin\\'

    source_full_path = bin_path  + flavor_release + '\\' + bin_file_name
    dest_full_path = nuget_installed + triplet + dest_path_segment + bin_file_name
    do_copy(source_full_path, dest_full_path)

# Copy debug version of dll, lib & pdb, not exe
    source_full_path = bin_path + flavor_debug + '\\' + bin_file_name
    dest_full_path = nuget_installed + triplet + '\\debug' + dest_path_segment + bin_file_name
    do_copy(source_full_path, dest_full_path)

def copy_v8_headers(triplet):
    source_folder = v8_source + 'include\\'
    dest_folder = nuget_installed + triplet + '\\include\\v8\\'
    filter = ''
    call(['robocopy', source_folder, dest_folder, filter, '/S', '/LOG:robocopy_includes.txt'])

def copy_jsi_ref_headers(triplet):
    source_folder = v8jsi_source + 'external_includes\\jsi_ref\\'
    dest_folder = nuget_installed + triplet + '\\include\\jsi_ref\\'
    filter = ''
    call(['robocopy', source_folder, dest_folder, filter, '/S', '/LOG:robocopy_includes.txt'])

def copy_jsi_rnw_headers(triplet):
    source_folder = v8jsi_source + 'external_includes\\jsi_rnw\\'
    dest_folder = nuget_installed + triplet + '\\include\\jsi_rnw\\'
    filter = ''
    call(['robocopy', source_folder, dest_folder, filter, '/S', '/LOG:robocopy_includes.txt'])

def copy_v8jsi_headers(triplet):
    source_folder = v8jsi_source + 'public\\'
    dest_folder = nuget_installed + triplet + '\\include\\jsiruntime\\'
    filter = ''
    call(['robocopy', source_folder, dest_folder, filter, '/S', '/LOG:robocopy_includes.txt'])


def do1():
    copy_v8jsi_bin('x64-windows', v8jsi_bin_x64, 'v8jsi.dll')
    copy_v8jsi_bin('x64-windows', v8jsi_bin_x64, 'v8jsi.pdb')
    copy_v8jsi_bin('x64-windows', v8jsi_bin_x64, 'v8jsi.lib')

    copy_v8jsi_bin('x86-windows', v8jsi_bin_x86, 'v8jsi.dll')
    copy_v8jsi_bin('x86-windows', v8jsi_bin_x86, 'v8jsi.pdb')
    copy_v8jsi_bin('x86-windows', v8jsi_bin_x86, 'v8jsi.lib')
    
    copy_v8jsi_headers('x64-windows')
    copy_v8jsi_headers('x86-windows')

    copy_v8_headers('x64-windows')
    copy_v8_headers('x86-windows')

    copy_jsi_ref_headers('x64-windows')
    copy_jsi_ref_headers('x86-windows')

    copy_jsi_rnw_headers('x64-windows')
    copy_jsi_rnw_headers('x86-windows')

    # source_folder = nuget_installed + 'x64-windows\\'
    # dest_folder = nuget_installed + 'x64-uwp\\'
    # filter = ''
    # call(['robocopy', source_folder, dest_folder, filter, '/S', '/LOG:robocopy_includes.txt'])

    # source_folder = nuget_installed + 'x86-windows\\'
    # dest_folder = nuget_installed + 'x86-uwp\\'
    # filter = ''
    # call(['robocopy', source_folder, dest_folder, filter, '/S', '/LOG:robocopy_includes.txt'])

def main():
    do1()

if __name__ == '__main__':
    main()