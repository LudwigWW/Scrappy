# Generated by Django 3.0.7 on 2020-07-19 23:17

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('customizer', '0012_scraptype_ui_name'),
    ]

    operations = [
        migrations.AddField(
            model_name='scrapobject',
            name='name_by_user',
            field=models.CharField(default='', max_length=200),
        ),
        migrations.AddField(
            model_name='scraptype',
            name='id_initials',
            field=models.CharField(default='', max_length=20),
        ),
        migrations.AlterField(
            model_name='scrapobject',
            name='scrap_id',
            field=models.CharField(max_length=200, unique=True),
        ),
    ]
