import subprocess
import json
import os
from django.shortcuts import get_object_or_404, render
from django.http import HttpResponse, HttpResponseRedirect, JsonResponse
from django.template import loader
from django.urls import reverse
from django.conf import settings
from django.core.files.base import ContentFile

from .models import Choice, Question, Slider, CustomizerClass, ScrapObject

from customizer.forms import PostForm, StorageForm, PreviewForm


def post_new(request):
    form = PostForm()
    return render(request, 'customizer/post_edit.html', {'form': form})


def index(request):
    latest_question_list = Question.objects.order_by('-pub_date')[:5]
    classes_list = CustomizerClass.objects.order_by('-class_name')[:5]
    classes_list[0]._meta.get_fields()
    print(classes_list[0]._meta.get_fields())
    print(classes_list[1].sliders.all())
    context = {'latest_question_list': latest_question_list, 'classes_list': classes_list}
    return render(request, 'customizer/index.html', context)

def detail(request, question_id):
    question = get_object_or_404(Question, pk=question_id)
    return render(request, 'customizer/detail.html', {'question': question})

def results(request, question_id):
    question = get_object_or_404(Question, pk=question_id)
    return render(request, 'customizer/results.html', {'question': question})

def vote(request, question_id):
    question = get_object_or_404(Question, pk=question_id)
    try:
        selected_choice = question.choice_set.get(pk=request.POST['choice'])
    except (KeyError, Choice.DoesNotExist):
        # Redisplay the question voting form.
        return render(request, 'customizer/detail.html', {
            'question': question,
            'error_message': "You didn't select a choice.",
        })
    else:
        selected_choice.votes += 1
        selected_choice.save()
        # Always return an HttpResponseRedirect after successfully dealing
        # with POST data. This prevents data from being posted twice if a
        # user hits the Back button.
        return HttpResponseRedirect(reverse('customizer:results', args=(question.id,)))

def execute_command(shell_command):
    p = subprocess.Popen(shell_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    cmd_output = ""
    
    return_value = p.wait()

    for line in p.stdout.readlines():
        cmd_output += str(line)[2:len(str(line))-3] + " \n"

    
    # print(return_value)
    # streamdata = p.communicate()[0]
    # rc = p.returncode
    # print(rc)
    return [return_value, cmd_output]

def scrapObjectDetail(request, scrapObject_id):
    scrapObject = get_object_or_404(ScrapObject, pk=scrapObject_id)
    storageForm = StorageForm()
    previewForm = PreviewForm()

    if request.method == "GET":
        getType = request.GET.get('getType', None)

        if getType == 'scrap_detail_page_image':
            imageVal = request.GET.get('imageVal', None)
            imageName = request.GET.get('imageName', None)
            delete = request.GET.get('delete', "NotFound")
            unfinished = True
            print("imageVal", imageVal)
            print("imageName", imageName)

            # form = PostForm(request.GET)

            if (imageName != None and (delete == True or imageVal != None)):
                if (imageName == 'preview_image'):
                    scrapObject.preview_image = imageVal
                    scrapObject.save()
                    unfinished = False
                elif (imageName == 'storage_picture'):
                    scrapObject.storage_picture = imageVal
                    scrapObject.save()
                    unfinished = False
            else:
                raise Exception("image data not available") 

            if unfinished:
                raise Exception("image field not found to save UI input data") 

            scrapObjectId = str(scrapObject.id)
            return JsonResponse({'scrapObjectId': scrapObjectId,
                                'successful':True})

    elif request.method == "POST":
        postType = request.POST.get('postType', None)
        if (postType == 'scrap_detail_page_save'):
            pass
        elif (postType == 'scrap_detail_page_upload_preview_image'):
            form = PreviewForm(request.POST, request.FILES)
            if form.is_valid():
                user_image_file = request.FILES.get('preview_image')
                print(user_image_file)
                scrapObject.preview_image = user_image_file
                scrapObject.save()
        elif (postType == 'scrap_detail_page_upload_storage_picture'):
            form = StorageForm(request.POST, request.FILES)
            if form.is_valid():
                user_image_file = request.FILES.get('storage_picture')
                print(user_image_file)
                scrapObject.storage_picture = user_image_file
                scrapObject.save()        
        else:
            raise Exception("Post type not recognized") 

    return render(request, 'customizer/scrapObjectDetail.html', {'scrapObject': scrapObject, 'storageForm': storageForm, 'previewForm': previewForm})

def STL_overview(request):
    scrapObjects = ScrapObject.objects.all()

    if request.method == "GET":
        getType = request.GET.get('getType', None)

        if getType == 'scrapObject_frequency_change':
            changedValue = request.GET.get('changedValue', False)
            changedObject = request.GET.get('changedObject', False)
            changedText = request.GET.get('changedText', False)
            print(changedValue)
            print(changedObject)
            print(changedText)
            unfinished = True

            if (changedValue != False and changedText != False):      
                for obj in scrapObjects:
                    if (changedObject == obj.scrap_id):
                        obj.frequency = changedValue
                        obj.frequency_text = changedText
                        obj.save()
                        print(obj.frequency_text)
                        unfinished = False
                        break
            else:
                raise Exception("Field data not available") 

            if unfinished:
                raise Exception("Field not found to save UI input data")

        if getType == 'scrapObject_number_stored_change':
            changedValue = request.GET.get('changedValue', False)
            changedObject = request.GET.get('changedObject', False)
            print(changedValue)
            print(changedObject)
            unfinished = True

            if (changedValue != False):      
                for obj in scrapObjects:
                    if (changedObject == obj.scrap_id):
                        obj.number_stored = changedValue
                        obj.save()
                        unfinished = False
                        break
            else:
                raise Exception("Field data not available") 

            if unfinished:
                raise Exception("Field not found to save UI input data")
            
    return render(request, 'customizer/STL_overview.html', {'scrapObjects': scrapObjects})

def customizerClass(request, customizerClass_id):
    print(request)
    customizerClass = get_object_or_404(CustomizerClass, pk=customizerClass_id)
    # print(customizerClass.sliders.all())

    # Handle POST
    if request.method == "POST":
        postType = request.POST.get('postType', None)
        if (postType == 'customizer_save_STL'):

            # update cusomizer values
            for slider in customizerClass.sliders.all():
                posted_value = request.POST.get(slider.input_name)
                if (posted_value):
                    print(posted_value)
                    slider.value = posted_value
                    slider.save()
                else:
                    print("No posted value!")

            for toggle in customizerClass.toggles.all():
                posted_value = request.POST.get(toggle.input_name, False)
                print('posted_value', posted_value)
                if (posted_value):
                    print(posted_value)
                    toggle.value = True
                    toggle.save()
                else:
                    toggle.value = False
                    toggle.save()
                    print("No posted value!")

            # write customizer values to parameter file
            write_parameters(customizerClass)

            # load parameter file into script file
            print(customizerClass.script_file.path)
            if not os.path.isfile(customizerClass.script_file.path):
                print("Script file not valid!")
            else:
                with customizerClass.script_file.open('r') as script_file:
                    script_lines = script_file.readlines()
                # print(script_lines)
                for line in script_lines:
                    try:
                        line = line.decode('utf-8')
                    except (UnicodeDecodeError, AttributeError):
                        pass
                if (len(script_lines) <= 2):
                    print("Script file too short! Fix byte encoding!")

                script_lines[0] = "include <" + str(settings.MEDIA_ROOT) + "/" + str(customizerClass.parameter_file.name) + ">;\n"
                with customizerClass.script_file.open('w') as script_file:
                    script_file.writelines(script_lines)

            # Preparation done, create new scrap object

            this_stl_count = str(customizerClass.stl_count)
            customizerClass.stl_count += 1
            customizerClass.save()

            new_ScrapObject = ScrapObject()
            new_ScrapObject.scrap_id = customizerClass.id_initials + this_stl_count



            new_ScrapObject.name_by_user = customizerClass.scrap_name_holder
            new_ScrapObject.storage_hint = customizerClass.scrap_description_holder
            new_ScrapObject.storage_picture = customizerClass.scrap_image_holder

            original_preview_path = settings.STATIC_DIR + '/img/' + str(customizerClass.class_name) + str(customizerClass.preview_counter) + '.png'
            new_preview_path = settings.MEDIA_ROOT + '/previews/' + str(customizerClass.class_name) + str(customizerClass.preview_counter) + '.png'
            print('original_preview_path', original_preview_path)
            print('new_preview_path', new_preview_path)
            # With closes properly also on exceptions
            with open(original_preview_path, "rb") as fh:
                # Get the content of the file, we also need to close the content file
                with ContentFile(fh.read()) as 	file_content:
                # Set the media attribute of the article, but under an other path/filename
                    new_ScrapObject.preview_image.save(new_preview_path, file_content)
                    # Save the article
                    new_ScrapObject.save()

            new_ScrapObject.preview_image = 'previews/' + str(customizerClass.class_name) + str(customizerClass.preview_counter) + '.png'
            print('new_ScrapObject.preview_image', new_ScrapObject.preview_image)
            new_ScrapObject.save()
            # TODO change static openscad calls to static URL
            shell_command = "openscad '" + customizerClass.script_file.path + "' -o 'static/STL/" + customizerClass.id_initials + "_" + this_stl_count + ".stl'"
            command_result = execute_command(shell_command)
            print(shell_command)
            print(command_result)

            original_stl_path = settings.STATIC_DIR + '/STL/' + customizerClass.id_initials + "_" + this_stl_count + ".stl"
            new_stl_path = settings.MEDIA_ROOT + '/STL/' + customizerClass.id_initials + "_" + this_stl_count + ".stl"

            # With closes properly also on exceptions
            with open(original_stl_path, "rb") as fh2:
                # Get the content of the file, we also need to close the content file
                with ContentFile(fh2.read()) as file_content2:
                # Set the media attribute of the article, but under an other path/filename
                    new_ScrapObject.stl_file.save(new_stl_path, file_content2)
                    # Save the article
                    new_ScrapObject.save()
            
            del fh
            del fh2
            
        
        elif (postType == 'customizer_upload_image'):

            form = PostForm(request.POST, request.FILES)
            if form.is_valid():
                user_image_file = request.FILES.get('scrap_image_holder')
                print(user_image_file)
                customizerClass.scrap_image_holder = user_image_file
                customizerClass.save()
        else:
            raise Exception("Post type not recognized") 


    # Handle GET
    elif request.method == "GET":
        getType = request.GET.get('getType', None)

        if getType == 'customizer_slider_change':
            sliderVal = request.GET.get('sliderVal', False)
            sliderName = request.GET.get('sliderName', False)
            unfinished = True
            print(sliderVal)
            print(sliderName)

            if (sliderVal != False and sliderName != False):      
                for slider in customizerClass.sliders.all():
                    print(slider.input_name)
                    if (sliderName == slider.input_name):
                        slider.value = sliderVal
                        slider.save()
                        print(sliderVal)
                        unfinished = False
                        break
            else:
                raise Exception("Slider data not available") 

            if unfinished:
                raise Exception("Slider not found to save UI input data") 

            return update_preview(request, customizerClass)

        elif getType == 'customizer_toggle_change':
            toggleVal = request.GET.get('toggleVal', "Wrong")
            toggleName = request.GET.get('toggleName', False)
            unfinished = True
            print("toggleVal", toggleVal)
            print("toggleName", toggleName)
            if (toggleVal == "false"):
                toggleVal = False
            elif (toggleVal == "true"):
                toggleVal = True

            if (toggleVal != "Wrong" and toggleName != False):      
                for toggle in customizerClass.toggles.all():
                    if (toggleName == toggle.input_name):
                        toggle.value = toggleVal
                        toggle.save()
                        print(toggleVal)
                        unfinished = False
                        break
            else:
                raise Exception("Toggle data not available") 

            if unfinished:
                raise Exception("Toggle not found to save UI input data") 

            return update_preview(request, customizerClass)

        elif getType == 'customizer_camera_click':
            cameraDirection = request.GET.get('cameraDirection', False)
            unfinished = True
            print("cameraDirection", cameraDirection)

            if (cameraDirection != None):
                if cameraDirection == 'up':
                    customizerClass.camera_x += 30
                    customizerClass.save()
                    unfinished = False
                elif cameraDirection == 'down':
                    customizerClass.camera_x -= 30
                    customizerClass.save()
                    unfinished = False
                elif cameraDirection == 'right':
                    customizerClass.camera_z += 30
                    customizerClass.save()
                    unfinished = False
                elif cameraDirection == 'left':
                    customizerClass.camera_z -= 30
                    customizerClass.save()
                    unfinished = False 
            else:
                raise Exception("camera click data not available") 

            if unfinished:
                raise Exception("direction not found to save UI input data") 

            return update_preview(request, customizerClass)
        
        elif getType == 'customizer_name_change':
            textVal = request.GET.get('textVal', False)
            textName = request.GET.get('textName', False)
            unfinished = True
            print("textVal", textVal)

            if (textName != None):
                if textName == 'scrap_name_holder':
                    customizerClass.scrap_name_holder = textVal
                    customizerClass.save()
                    unfinished = False
                elif textName == 'scrap_description_holder':
                    customizerClass.scrap_description_holder = textVal
                    customizerClass.save()
                    unfinished = False
            else:
                raise Exception("text data not available") 

            if unfinished:
                raise Exception("text value field not found to save UI input data") 

            return JsonResponse({'successful':True})

        elif getType == 'customizer_image':
            imageVal = request.GET.get('imageVal', None)
            imageName = request.GET.get('imageName', None)
            delete = request.GET.get('delete', "NotFound")
            unfinished = True
            print("imageVal", imageVal)
            print("imageName", imageName)

            # form = PostForm(request.GET)

            if (imageName != None and (delete == True or imageVal != None)):
                if (imageName == 'scrap_image_holder'):
                    customizerClass.scrap_image_holder = imageVal
                    customizerClass.save()
                    unfinished = False
            else:
                raise Exception("image data not available") 

            if unfinished:
                raise Exception("image field not found to save UI input data") 

            customizerClassId = str(customizerClass.id)
            return JsonResponse({'customizerClassId': customizerClassId,
                                'successful':True})

    imageForm = PostForm()
    return render(request, 'customizer/customizerClass.html', {'customizerClass': customizerClass, 'imageForm': imageForm})

def update_preview(request, customizerClass):
    write_parameters(customizerClass)

    # delete old preview image
    if os.path.exists("static/img/" + customizerClass.class_name + str(customizerClass.preview_counter) + ".png"):
        os.remove("static/img/" + customizerClass.class_name + str(customizerClass.preview_counter) + ".png")

    customizerClass.preview_counter += 1
    if (customizerClass.preview_counter > 100000):
        customizerClass.preview_counter = 0
    customizerClass.save()

    shell_command = "openscad '" + customizerClass.script_file.path + "' -o 'static/img/" + customizerClass.class_name + str(customizerClass.preview_counter) + ".png' --camera 0,0,0," + str(customizerClass.camera_x) +","+ str(customizerClass.camera_y) +","+ str(customizerClass.camera_z)+",500 --viewall --autocenter"
    command_result = execute_command(shell_command)
    print(shell_command)
    print(command_result)

    preview_image_name = customizerClass.class_name + str(customizerClass.preview_counter)
    customizerClassId = str(customizerClass.id)
    print(preview_image_name)

    data = {
        #'is_taken': User.objects.filter(username__iexact=username).exists()
        'customizerClassId': customizerClassId,
        'preview_image' : preview_image_name
    }
    return JsonResponse(data)

def write_parameters(customizerClass):
    if not os.path.isfile(customizerClass.parameter_file.path):
        print(customizerClass.parameter_file.path)
        print("Parameter file not valid!")
        return False
    else:
        with customizerClass.parameter_file.open('w') as parameter_file:
            for slider in customizerClass.sliders.all():
                print(str(slider.value))
                parameter_file.write(slider.input_name + " = " + str(slider.value) + ";\n")
            for toggle in customizerClass.toggles.all():
                outString = "false"
                if (toggle.value):
                    outString = "true"
                parameter_file.write(toggle.input_name + " = " + str(outString) + ";\n")

        return True

def update_customizer(request):
    sliderVal = request.GET.get('sliderVal', None)
    print("Running ", sliderVal)
    is_big = False
    if (sliderVal >= 150):
        is_big = True
    data = {
        'transmitted_value' : sliderVal
    }
    return JsonResponse(data)

