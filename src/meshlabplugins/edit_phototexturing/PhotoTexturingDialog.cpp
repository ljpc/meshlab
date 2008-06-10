#include <QtGui>
 #include <QList>
#include <PhotoTexturingDialog.h>
#include <meshlab/mainwindow.h>
#include <wrap/gl/trimesh.h>
#include <meshlab/stdpardialog.h>

PhotoTexturingDialog::PhotoTexturingDialog(MeshEditInterface* plugin, PhotoTexturer* texturer,MeshModel &m,GLArea *gla): QDialog() {
	
	connect(this,SIGNAL(updateGLAreaTextures()),gla,SLOT(updateTexture()));
	connect(this,SIGNAL(setGLAreaTextureMode(vcg::GLW::TextureMode)),gla,SLOT(setTextureMode(vcg::GLW::TextureMode)));
	connect(this,SIGNAL(updateMainWindowMenus()),gla,SIGNAL(updateMainWindowMenus()));

	ptPlugin = plugin;
	photoTexturer = texturer;
	PhotoTexturingDialog::ui.setupUi(this);
	
	mesh = &m;
	glarea = gla;
	
	//setting up the headers for the tblewidget
	QStringList headers;
	headers << "Camera" << "Image";
	ui.cameraTableWidget->setHorizontalHeaderLabels(headers);
	
	connect(ui.configurationLoadPushButton, SIGNAL(clicked()),this,SLOT(loadConfigurationFile()));
	connect(ui.configurationSavePushButton, SIGNAL(clicked()),this,SLOT(saveConfigurationFile()));
	connect(ui.addCameraPushButton, SIGNAL(clicked()),this,SLOT(addCamera()));
	connect(ui.removeCameraPushButton, SIGNAL(clicked()),this,SLOT(removeCamera()));

	connect(ui.assignImagePushButton, SIGNAL(clicked()),this,SLOT(assignImage()));
	connect(ui.calculateTexturesPushButton, SIGNAL(clicked()),this,SLOT(calculateTextures()));
	connect(ui.combineTexturesPushButton, SIGNAL(clicked()),this,SLOT(combineTextures()));
	connect(ui.selectCurrentTextureComboBox, SIGNAL(activated(int )),this,SLOT(selectCurrentTexture(int )));
	
	connect(ui.applyPushButton, SIGNAL(clicked()),this,SLOT(apply()));
	connect(ui.closePushButton, SIGNAL(clicked()),this,SLOT(close()));
	connect(ui.cancelPushButton, SIGNAL(clicked()),this,SLOT(cancel()));
	
	photoTexturer->storeOriginalTextureCoordinates(mesh);
	
	update();
	
}


PhotoTexturingDialog::~PhotoTexturingDialog(){

}
void PhotoTexturingDialog::loadConfigurationFile(){
	QString filename = QFileDialog::getOpenFileName(this,tr("Select Configuration File"),".", "*.ptcfg");
	ui.calibrationFileLineEdit->setText(filename);
	photoTexturer->loadConfigurationFile(filename);
	update();
}

void PhotoTexturingDialog::saveConfigurationFile(){
	QString filename = QFileDialog::getSaveFileName(this,tr("Select Configuration File"),".", "*.ptcfg");
	ui.calibrationFileLineEdit->setText(filename);
	photoTexturer->saveConfigurationFile(filename);
}

void PhotoTexturingDialog::addCamera(){
	QString filename = QFileDialog::getOpenFileName(this,tr("Select Calibration File"),".", "Cameras (*.tsai)");
	photoTexturer->addCamera(filename);
	update();
	ui.cameraTableWidget->selectRow(ui.cameraTableWidget->rowCount()-1);
}

void PhotoTexturingDialog::removeCamera(){
	//int selectedRow = ui.cameraTableWidget->sel
	QList <QTableWidgetItem*>list = ui.cameraTableWidget->selectedItems();
	if (list.size()>0){
		QTableWidgetItem* item = list.at(0);
		int row = item->row();
		printf("row: %d\n",row);
		photoTexturer->removeCamera(row);
		update();
	}
	
}

void PhotoTexturingDialog::update(){
	int rowcount = photoTexturer->cameras.size();
	ui.cameraTableWidget->setRowCount((rowcount));
	int i;
	int currentIdx = ui.selectCurrentTextureComboBox->currentIndex();
	ui.selectCurrentTextureComboBox->clear();
	for (i=0;i<rowcount;i++){
		QString camname;
		QString imagename;
		Camera *cam = photoTexturer->cameras.at(i);
		camname =cam->name;
		imagename = cam->textureImage;
		QTableWidgetItem *camTypeItem = new QTableWidgetItem(camname);
		ui.cameraTableWidget->setItem(i, 0, camTypeItem);
		QTableWidgetItem *textureImageItem = new QTableWidgetItem(imagename);
		ui.cameraTableWidget->setItem(i, 1, textureImageItem);
		if (cam->calculatedTextures){
			ui.selectCurrentTextureComboBox->addItem(camname,QVariant(i));
		}
	}
	if (currentIdx < ui.selectCurrentTextureComboBox->count()){
		ui.selectCurrentTextureComboBox->setCurrentIndex(currentIdx);
	}
	ui.cameraTableWidget->resizeColumnsToContents();

	
	if (vcg::tri::HasPerFaceAttribute(mesh->cm,PhotoTexturer::ORIGINALUVTEXTURECOORDS) && vcg::tri::HasPerFaceAttribute(mesh->cm,PhotoTexturer::CAMERAUVTEXTURECOORDS)){
		ui.combineTexturesPushButton->setDisabled(false);
	}else{
		ui.combineTexturesPushButton->setDisabled(true);
	}
	
}
void PhotoTexturingDialog::assignImage(){
	QString filename = QFileDialog::getOpenFileName(this,tr("Select Image File"),".", "Images (*.png *.jpg *.bmp)");
	QList <QTableWidgetItem*>list = ui.cameraTableWidget->selectedItems();
	if (list.size()>0){
		QTableWidgetItem* item = list.at(0);
		int row = item->row();
		Camera *cam = photoTexturer->cameras.at(row);
		cam->textureImage = filename;
		update();
	}
	
}
void PhotoTexturingDialog::calculateTextures(){
	photoTexturer->calculateMeshTextureForAllCameras(mesh);
	glarea->update();
	update();
	
	updateGLAreaTextures();
	
	updateMainWindowMenus();
}

void PhotoTexturingDialog::selectCurrentTexture(int index){
	int icam = ui.selectCurrentTextureComboBox->itemData(index).toInt();
	photoTexturer->applyTextureToMesh(mesh,icam);
	setGLAreaTextureMode(vcg::GLW::TMPerWedgeMulti);
	glarea->update();
}

void PhotoTexturingDialog::combineTextures(){
	FilterParameterSet combineParamSet;
	combineParamSet.addInt("width",1024,"Image width:","");
	combineParamSet.addInt("height",1024,"Image height:","");
	//combineParamSet.addBool("saveImages",true,"Save all images","");
	//combineParamSet.addBool("sameFolder",true,"Same folder as mesh","");
	//buildParameterSet(alignParamSet, defaultAP);
							
	GenericParamDialog ad(this,&combineParamSet,"Texture Baking Parameters");
	int result=ad.exec();
	if (result == 1){
		photoTexturer->combineTextures(mesh,combineParamSet.getInt("width"),combineParamSet.getInt("height"));
	}
	update();
}

void PhotoTexturingDialog::apply(){
	
}
void PhotoTexturingDialog::close(){
	//glarea->endEdit();
	//ptPlugin->EndEdit(NULL,NULL,NULL);
}
void PhotoTexturingDialog::cancel(){
	photoTexturer->restoreOriginalTextureCoordinates(mesh);
	glarea->update();
}
